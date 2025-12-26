#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/dfu/mcuboot.h>

#include "aws_iot_mqtt.h"
#include "json_payload.h"
#include "common.h"
#include <zephyr/data/json.h>
#include <zephyr/types.h>
#include <cJSON.h>

#include "scd41.h"
#include "sht40.h"

LOG_MODULE_REGISTER(AWS_IOT_MQTT);

#define MODEM_FIRMWARE_VERSION_SIZE_MAX 50

bool OTA_STARTED = false;
bool MQTT_INIT = false;
bool AWS_IOT_MQTT_CONNECTED = false;
bool AWS_IOT_MQTT_DISCONNECTED = false;
bool SHADOW_PUBLISHED = false;
bool AWS_IOT_FINISHED = false;

char DEVICE_THING_NAME[HW_ID_LEN];

static void shadow_update_work_fn(struct k_work *work);
static void connect_work_fn(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(shadow_update_work, shadow_update_work_fn);
static K_WORK_DELAYABLE_DEFINE(connect_work, connect_work_fn);

char topic_accepted[128];
char topic_rejected[128];
char topic_delta[128];

extern bool device_sleep;

int get_device_id_imei(void)
{
    int err = hw_id_get(DEVICE_THING_NAME, HW_ID_LEN); // Get device imei as thing name for aws iot

    if (err != 0)
    {
        LOG_ERR("Failed to get Device ID IMEI\n\r");
    }
    LOG_INF("Device ID IMEI. [THING NAME] : %s", DEVICE_THING_NAME);
    return err;
}

static int app_topics_subscribe(void)
{
    int err;
    static const struct mqtt_topic topic_list[] = {
        {
            .topic.utf8 = MY_CUSTOM_TOPIC_1,
            .topic.size = sizeof(MY_CUSTOM_TOPIC_1) - 1,
            .qos = MQTT_QOS_1_AT_LEAST_ONCE,
        },
        {
            .topic.utf8 = MY_CUSTOM_TOPIC_2,
            .topic.size = sizeof(MY_CUSTOM_TOPIC_2) - 1,
            .qos = MQTT_QOS_1_AT_LEAST_ONCE,
        },
        {
            .topic.utf8 = SENSOR_SHADOW_TOPIC_ACCEPTED,
            .topic.size = sizeof(SENSOR_SHADOW_TOPIC_ACCEPTED) - 1,
            .qos = MQTT_QOS_1_AT_LEAST_ONCE,
        },
        {
            .topic.utf8 = SENSOR_SHADOW_TOPIC_REJECTED,
            .topic.size = sizeof(SENSOR_SHADOW_TOPIC_REJECTED) - 1,
            .qos = MQTT_QOS_1_AT_LEAST_ONCE,
        },
        // {
        //     .topic.utf8 = SENSOR_SHADOW_TOPIC_DELTA,
        //     .topic.size = sizeof(SENSOR_SHADOW_TOPIC_DELTA) - 1,
        //     .qos = MQTT_QOS_1_AT_LEAST_ONCE,
        // },
        // {
        //     .topic.utf8 = SENSOR_SHADOW_TOPIC_UPDATE,
        //     .topic.size = sizeof(SENSOR_SHADOW_TOPIC_UPDATE) - 1,
        //     .qos = MQTT_QOS_1_AT_LEAST_ONCE,
        // },
    };

    err = aws_iot_application_topics_set(topic_list, ARRAY_SIZE(topic_list));
    if (err)
    {
        LOG_ERR("aws_iot_application_topics_set, error: %d", err);
        FATAL_ERROR();
        return err;
    }

    return 0;
}

static void shadow_update_work_fn(struct k_work *work)
{
    int err;
    char message[CONFIG_AWS_IOT_JSON_MESSAGE_SIZE_MAX] = {0};

    // Constructing the payload with the updated configuration values for the reported state
    struct payload payload = {
        .state.reported.sleep_time = device_sleep_time_minutes,  // Current sleep time
        .state.reported.sensor_co2_enable = sensor_co2_enable,    // CO2 sensor enable state
        .state.reported.co2_medium_threshold = co2_medium_threshold, // CO2 medium threshold
        .state.reported.co2_high_threshold = co2_high_threshold,  // CO2 high threshold
        .state.reported.temp_high_threshold = temp_high_threshold, // Temperature high threshold
        .state.reported.temp_low_threshold = temp_low_threshold,   // Temperature low threshold
        .state.reported.hum_high_threshold = hum_high_threshold,   // Humidity high threshold
        .state.reported.hum_low_threshold = hum_low_threshold,     // Humidity low threshold
    };

    // Create the topic to update the device shadow
    char topic[128];
    snprintf(topic, sizeof(topic), "$aws/things/%s/shadow/update", DEVICE_THING_NAME);

    struct aws_iot_data tx_data = {
        .qos = MQTT_QOS_1_AT_LEAST_ONCE,
        .topic.type = AWS_IOT_SHADOW_TOPIC_UPDATE,
    };

    // Construct the shadow update payload as a JSON message
    err = json_payload_construct(message, sizeof(message), &payload);
    if (err)
    {
        LOG_ERR("json_payload_construct failed, error: %d", err);
        return;
    }

    tx_data.ptr = message;
    tx_data.len = strlen(message);

    LOG_INF("Publishing Shadow Update:");
    LOG_INF("DATA : %s", message);
    LOG_WRN("Shadow Update Topic: %s", topic);

    // Send the shadow update
    err = aws_iot_send(&tx_data);
    if (err)
    {
        LOG_ERR("aws_iot_send failed, error: %d", err);
        return;
    }

    // Schedule the next shadow update
    (void)k_work_reschedule(&shadow_update_work, K_SECONDS(CONFIG_AWS_IOT_PUBLICATION_INTERVAL_SECONDS));

    SHADOW_PUBLISHED = true;
}



static void connect_work_fn(struct k_work *work)
{
    int err;
    const struct aws_iot_config config = {
        .client_id = DEVICE_THING_NAME,
    };

    LOG_INF("Connecting to AWS IoT");

    err = aws_iot_connect(&config);
    if (err == -EAGAIN)
    {
        LOG_INF("Connection attempt timed out, "
                "Next connection retry in %d seconds",
                CONFIG_AWS_IOT_CONNECTION_RETRY_TIMEOUT_SECONDS);

        (void)k_work_reschedule(&connect_work,
                                K_SECONDS(CONFIG_AWS_IOT_CONNECTION_RETRY_TIMEOUT_SECONDS));
    }
    else if (err)
    {
        LOG_ERR("aws_iot_connect, error: %d", err);
        FATAL_ERROR();
    }
}

static void on_aws_iot_evt_connected(const struct aws_iot_evt *const evt)
{
    (void)k_work_cancel_delayable(&connect_work);

    /* If persistent session is enabled, the AWS IoT library will not subscribe to any topics.
     * Topics from the last session will be used.
     */
    if (evt->data.persistent_session)
    {
        LOG_WRN("Persistent session is enabled, using subscriptions "
                "from the previous session");
    }

    /* Mark image as working to avoid reverting to the former image after a reboot. */
#if defined(CONFIG_BOOTLOADER_MCUBOOT)
    boot_write_img_confirmed();
#endif

    if (!device_sleep)
    {
        /* Start sequential updates to AWS IoT. */
        (void)k_work_reschedule(&shadow_update_work, K_NO_WAIT);
    }
}

static void on_aws_iot_evt_disconnected(void)
{
    (void)k_work_cancel_delayable(&shadow_update_work);
    (void)k_work_reschedule(&connect_work, K_SECONDS(5));
}

static void on_aws_iot_evt_fota_done(const struct aws_iot_evt *const evt)
{
    /* Tear down MQTT connection. */
    (void)aws_iot_disconnect();
    (void)k_work_cancel_delayable(&connect_work);

    if (evt->data.image & DFU_TARGET_IMAGE_TYPE_ANY_APPLICATION)
    {
        LOG_INF("Application FOTA done, rebooting");
        IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)));
    }
    else
    {
        LOG_WRN("Unexpected FOTA image type");
    }
}

void on_net_event_l4_connected(void)
{
    (void)k_work_reschedule(&connect_work, K_SECONDS(5));
}

void on_net_event_l4_disconnected(void)
{
    // (void)aws_iot_disconnect();
    (void)k_work_cancel_delayable(&connect_work);
    (void)k_work_cancel_delayable(&shadow_update_work);
}

void handle_shadow_delta(const char *json, size_t len)
{
    cJSON *root = cJSON_ParseWithLength(json, len);
    if (!root)
    {
        LOG_ERR("Failed to parse JSON");
        return;
    }

    cJSON *state = cJSON_GetObjectItem(root, "state");
    if (!state)
    {
        LOG_ERR("No 'state' object in delta");
        cJSON_Delete(root);
        return;
    }

    // Parse existing ones
    cJSON *sleep_time = cJSON_GetObjectItem(state, "sleep_time");
    if (sleep_time && cJSON_IsNumber(sleep_time)) {
        device_sleep_time_minutes = sleep_time->valueint;
        LOG_INF("Updating sleep_time to %d", device_sleep_time_minutes);
    }

    cJSON *sensor_enable = cJSON_GetObjectItem(state, "sensor_enable");
    if (sensor_enable && cJSON_IsBool(sensor_enable)) {
        sensor_co2_enable = cJSON_IsTrue(sensor_enable);
        LOG_INF("Updating sensor_enable to %d", sensor_co2_enable);
    }

    // ✅ New: Parse thresholds
    cJSON *co2_med = cJSON_GetObjectItem(state, "co2_medium_threshold");
    if (co2_med && cJSON_IsNumber(co2_med)) {
        co2_medium_threshold = co2_med->valuedouble;
        LOG_INF("Updated co2_medium_threshold: %.2f", co2_medium_threshold);
    }

    cJSON *co2_high = cJSON_GetObjectItem(state, "co2_high_threshold");
    if (co2_high && cJSON_IsNumber(co2_high)) {
        co2_high_threshold = co2_high->valuedouble;
        LOG_INF("Updated co2_high_threshold: %.2f", co2_high_threshold);
    }

    cJSON *temp_high = cJSON_GetObjectItem(state, "temp_high_threshold");
    if (temp_high && cJSON_IsNumber(temp_high)) {
        temp_high_threshold = temp_high->valuedouble;
        LOG_INF("Updated temp_high_threshold: %.2f", temp_high_threshold);
    }

    cJSON *temp_low = cJSON_GetObjectItem(state, "temp_low_threshold");
    if (temp_low && cJSON_IsNumber(temp_low)) {
        temp_low_threshold = temp_low->valuedouble;
        LOG_INF("Updated temp_low_threshold: %.2f", temp_low_threshold);
    }

    cJSON *hum_high = cJSON_GetObjectItem(state, "hum_high_threshold");
    if (hum_high && cJSON_IsNumber(hum_high)) {
        hum_high_threshold = hum_high->valuedouble;
        LOG_INF("Updated hum_high_threshold: %.2f", hum_high_threshold);
    }

    cJSON *hum_low = cJSON_GetObjectItem(state, "hum_low_threshold");
    if (hum_low && cJSON_IsNumber(hum_low)) {
        hum_low_threshold = hum_low->valuedouble;
        LOG_INF("Updated hum_low_threshold: %.2f", hum_low_threshold);
    }

    cJSON_Delete(root);
}

int aws_iot_publish_topic(const char *topic,
                          const char *payload,
                          enum mqtt_qos qos)
{
    if (!topic || !payload)
    {
        LOG_ERR("Topic or payload is NULL");
        return -EINVAL;
    }

    struct aws_iot_topic_data topic_data = {
        .str = topic,
        .len = strlen(topic)};

    struct aws_iot_data msg = {
        .ptr = (char *)payload,
        .len = strlen(payload),
        .qos = qos,
        .topic = topic_data,
    };

    int err = aws_iot_send(&msg);
    if (err)
    {
        LOG_ERR("aws_iot_send failed: %d", err);
        return err;
    }

    LOG_INF("Publishing DATA :");
    LOG_WRN("TOPIC [ %s ]", topic);
    LOG_INF("%s\n", payload);
    return 0;
}

static void aws_iot_event_handler(const struct aws_iot_evt *const evt)
{
    switch (evt->type)
    {
    case AWS_IOT_EVT_CONNECTING:
        LOG_INF("AWS_IOT_EVT_CONNECTING\n\r");
        break;
    case AWS_IOT_EVT_CONNECTED:
        LOG_INF("AWS_IOT_EVT_CONNECTED\n\r");
        AWS_IOT_MQTT_CONNECTED = true;
        AWS_IOT_MQTT_DISCONNECTED = false;
        on_aws_iot_evt_connected(evt);
        break;
    case AWS_IOT_EVT_DISCONNECTED:
        LOG_INF("AWS_IOT_EVT_DISCONNECTED\n\r");
        AWS_IOT_MQTT_DISCONNECTED = true;
        AWS_IOT_MQTT_CONNECTED = false;
        on_aws_iot_evt_disconnected();
        break;
    case AWS_IOT_EVT_DATA_RECEIVED:
        LOG_INF("AWS_IOT_EVT_DATA_RECEIVED:");
        LOG_INF("DATA : \"%.*s\"", evt->data.msg.len, evt->data.msg.ptr);
        LOG_WRN("TOPIC : \"%.*s\"", evt->data.msg.topic.len, evt->data.msg.topic.str);
        char topic[128];
        sprintf(topic, "$aws/things/%s/shadow/update/delta", DEVICE_THING_NAME);
        if (strcmp(evt->data.msg.topic.str, topic) == 0)
        {
            handle_shadow_delta(evt->data.msg.ptr, evt->data.msg.len);
        }
        break;
    case AWS_IOT_EVT_PUBACK:
        LOG_INF("AWS_IOT_EVT_PUBACK, message ID: %d\n\r", evt->data.message_id);
        break;
    case AWS_IOT_EVT_PINGRESP:
        LOG_INF("AWS_IOT_EVT_PINGRESP\n\r");
        break;
    case AWS_IOT_EVT_FOTA_START:
        OTA_STARTED = true;
        LOG_INF("AWS_IOT_EVT_FOTA_START\n\r");
        break;
    case AWS_IOT_EVT_FOTA_ERASE_PENDING:
        LOG_INF("AWS_IOT_EVT_FOTA_ERASE_PENDING\n\r");
        break;
    case AWS_IOT_EVT_FOTA_ERASE_DONE:
        LOG_INF("AWS_FOTA_EVT_ERASE_DONE\n\r");
        break;
    case AWS_IOT_EVT_FOTA_DONE:
        LOG_INF("AWS_IOT_EVT_FOTA_DONE\n\r");
        OTA_STARTED = false;
        on_aws_iot_evt_fota_done(evt);
        break;
    case AWS_IOT_EVT_FOTA_DL_PROGRESS:
        LOG_INF("AWS_IOT_EVT_FOTA_DL_PROGRESS, (%d%%)", evt->data.fota_progress);
        break;
    case AWS_IOT_EVT_ERROR:
        LOG_INF("AWS_IOT_EVT_ERROR, %d\n\r", evt->data.err);
        FATAL_ERROR();
        break;
    case AWS_IOT_EVT_FOTA_ERROR:
        OTA_STARTED = false;
        LOG_INF("AWS_IOT_EVT_FOTA_ERROR\n\r");
        break;
    default:
        LOG_WRN("Unknown AWS IoT event type: %d\n\r", evt->type);
        break;
    }
}

void aws_iot_cancel_shadow_work(void)
{
    (void)k_work_cancel_delayable(&connect_work);
    (void)k_work_cancel_delayable(&shadow_update_work);
    SHADOW_PUBLISHED = false;
}

int aws_iot_client_init(void)
{
    int err;

    err = get_device_id_imei();
    if (err != 0)
    {
        return err;
    }
    if (MQTT_INIT)
    {
        return 0;
    }

    err = aws_iot_init(aws_iot_event_handler);
    if (err)
    {
        LOG_ERR("AWS IoT library could not be initialized, error: %d", err);
        FATAL_ERROR();
        return err;
    }

    /* Add application specific non-shadow topics to the AWS IoT library.
     * These topics will be subscribed to when connecting to the broker.
     */
    err = app_topics_subscribe();
    if (err)
    {
        LOG_ERR("Adding application specific topics failed, error: %d", err);
        FATAL_ERROR();
        return err;
    }

    MQTT_INIT = true;

    return 0;
}

int aws_iot_mqtt_disconnect(void)
{
    int err = aws_iot_disconnect();
    if (err)
    {
        LOG_ERR("Failed to diconnect AWS iot MQTT %d", err);
        return err;
    }

    LOG_WRN("AWS IoT MQTT Disconnected \n\r");
    return err;
}