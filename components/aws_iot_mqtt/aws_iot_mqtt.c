#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <cJSON.h>

#include "aws_iot_mqtt.h"
#include "json_payload.h"
#include "common.h"

LOG_MODULE_REGISTER(AWS_IOT_MQTT);

bool OTA_STARTED = false;
bool MQTT_INIT = false;
bool AWS_IOT_MQTT_CONNECTED = false;
bool AWS_IOT_MQTT_DISCONNECTED = false;
bool SHADOW_PUBLISHED = false;
bool AWS_IOT_FINISHED = false;

char DEVICE_THING_NAME[HW_ID_LEN];

extern bool device_sleep;

/* Your globals (already exist in your project) */
extern uint32_t device_sleep_time_minutes;
extern bool sensor_co2_enable; /* mapped to sensor_enable */

static void shadow_update_work_fn(struct k_work *work);
static void connect_work_fn(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(shadow_update_work, shadow_update_work_fn);
static K_WORK_DELAYABLE_DEFINE(connect_work, connect_work_fn);

/* Topics (minimal) */
static char topic_shadow_delta[160];
static char topic_shadow_get_accepted[160];
static char topic_shadow_get_rejected[160];
static char topic_telemetry_pub[128]; /* only for your main.c publish */

/* ---------------- Helpers ---------------- */

static int build_shadow_topic(char *buf, size_t buflen, const char *suffix)
{
	int n = snprintk(buf, buflen, "$aws/things/%s/shadow/%s", DEVICE_THING_NAME, suffix);
	if (n < 0 || (size_t)n >= buflen) {
		return -ENOMEM;
	}
	return 0;
}

static int build_app_topic(char *buf, size_t buflen, const char *suffix)
{
	int n = snprintk(buf, buflen, "aws/things/%s/%s", DEVICE_THING_NAME, suffix);
	if (n < 0 || (size_t)n >= buflen) {
		return -ENOMEM;
	}
	return 0;
}

const char *aws_iot_get_telemetry_pub_topic(void)
{
	return topic_telemetry_pub;
}

static int get_device_id_imei(void)
{
	int err = hw_id_get(DEVICE_THING_NAME, HW_ID_LEN);
	DEVICE_THING_NAME[HW_ID_LEN - 1] = '\0';

	if (err) {
		LOG_ERR("Failed to get Device ID (HW ID/IMEI). err=%d", err);
		return err;
	}

	LOG_INF("Device ID IMEI. [THING NAME] : %s", DEVICE_THING_NAME);
	return 0;
}

/* ---------------- Shadow GET (important for sleep/reconnect) ---------------- */

static void request_shadow_get(void)
{
	char topic[160];
	int n = snprintk(topic, sizeof(topic), "$aws/things/%s/shadow/get", DEVICE_THING_NAME);
	if (n < 0 || (size_t)n >= sizeof(topic)) {
		LOG_ERR("Shadow GET topic build failed");
		return;
	}

	/* GET request: empty payload */
	(void)aws_iot_publish_topic(topic, "", MQTT_QOS_1_AT_LEAST_ONCE);
	LOG_INF("Requested shadow GET: %s", topic);
}

/* ---------------- Apply desired from GET accepted ---------------- */

static void handle_shadow_get_accepted_simple(const char *json, size_t len)
{
	cJSON *root = cJSON_ParseWithLength(json, len);
	if (!root) {
		LOG_ERR("GET accepted JSON parse failed");
		return;
	}

	cJSON *state = cJSON_GetObjectItem(root, "state");
	if (!state) {
		cJSON_Delete(root);
		return;
	}

	cJSON *desired = cJSON_GetObjectItem(state, "desired");
	if (!desired) {
		LOG_WRN("GET accepted: no desired");
		cJSON_Delete(root);
		return;
	}

	cJSON *sleep_time = cJSON_GetObjectItem(desired, "sleep_time");
	if (sleep_time && cJSON_IsNumber(sleep_time)) {
		device_sleep_time_minutes = (uint32_t)sleep_time->valueint;
		LOG_INF("GET: sleep_time -> %u", device_sleep_time_minutes);
	}

	cJSON *sensor_enable = cJSON_GetObjectItem(desired, "sensor_enable");
	if (sensor_enable && cJSON_IsBool(sensor_enable)) {
		sensor_co2_enable = cJSON_IsTrue(sensor_enable);
		LOG_INF("GET: sensor_enable -> %d", sensor_co2_enable);
	}

	cJSON_Delete(root);

	/* publish reported immediately */
	(void)k_work_reschedule(&shadow_update_work, K_NO_WAIT);
}

/* ---------------- Apply desired from DELTA ---------------- */

static void handle_shadow_delta_simple(const char *json, size_t len)
{
	cJSON *root = cJSON_ParseWithLength(json, len);
	if (!root) {
		LOG_ERR("Delta JSON parse failed");
		return;
	}

	cJSON *state = cJSON_GetObjectItem(root, "state");
	if (!state) {
		cJSON_Delete(root);
		return;
	}

	cJSON *sleep_time = cJSON_GetObjectItem(state, "sleep_time");
	if (sleep_time && cJSON_IsNumber(sleep_time)) {
		device_sleep_time_minutes = (uint32_t)sleep_time->valueint;
		LOG_INF("DELTA: sleep_time -> %u", device_sleep_time_minutes);
	}

	cJSON *sensor_enable = cJSON_GetObjectItem(state, "sensor_enable");
	if (sensor_enable && cJSON_IsBool(sensor_enable)) {
		sensor_co2_enable = cJSON_IsTrue(sensor_enable);
		LOG_INF("DELTA: sensor_enable -> %d", sensor_co2_enable);
	}

	cJSON_Delete(root);

	(void)k_work_reschedule(&shadow_update_work, K_NO_WAIT);
}

/* ---------------- Subscribe topics (MINIMAL to avoid -ENOMEM) ---------------- */

static int app_topics_subscribe(void)
{
	int err;

	/* minimal shadow topics */
	err = build_shadow_topic(topic_shadow_delta, sizeof(topic_shadow_delta), "update/delta");
	if (err) return err;

	err = build_shadow_topic(topic_shadow_get_accepted, sizeof(topic_shadow_get_accepted), "get/accepted");
	if (err) return err;

	err = build_shadow_topic(topic_shadow_get_rejected, sizeof(topic_shadow_get_rejected), "get/rejected");
	if (err) return err;

	/* telemetry publish topic used by your main.c */
	err = build_app_topic(topic_telemetry_pub, sizeof(topic_telemetry_pub), "telemetry/publish");
	if (err) return err;

	static struct mqtt_topic topic_list[] = {
		{ .topic = { .utf8 = topic_shadow_delta, .size = 0 }, .qos = MQTT_QOS_1_AT_LEAST_ONCE },
		{ .topic = { .utf8 = topic_shadow_get_accepted, .size = 0 }, .qos = MQTT_QOS_1_AT_LEAST_ONCE },
		{ .topic = { .utf8 = topic_shadow_get_rejected, .size = 0 }, .qos = MQTT_QOS_1_AT_LEAST_ONCE },
	};

	for (size_t i = 0; i < ARRAY_SIZE(topic_list); i++) {
		topic_list[i].topic.size = strlen(topic_list[i].topic.utf8);
	}

	err = aws_iot_application_topics_set(topic_list, ARRAY_SIZE(topic_list));
	if (err) {
		LOG_ERR("aws_iot_application_topics_set, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	LOG_INF("Subscribed topics (minimal):");
	LOG_INF("  %s", topic_shadow_delta);
	LOG_INF("  %s", topic_shadow_get_accepted);
	LOG_INF("  %s", topic_shadow_get_rejected);

	return 0;
}

/* ---------------- Shadow update publish (reported) ---------------- */

static void shadow_update_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	int err;
	char message[CONFIG_AWS_IOT_JSON_MESSAGE_SIZE_MAX];

	memset(message, 0, sizeof(message));

	struct payload payload = {
		.state.reported.sleep_time = device_sleep_time_minutes,
		.state.reported.sensor_enable = sensor_co2_enable,
	};

	struct aws_iot_data tx_data = {
		.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		.topic.type = AWS_IOT_SHADOW_TOPIC_UPDATE,
	};

	err = json_payload_construct(message, sizeof(message), &payload);
	if (err) {
		LOG_ERR("json_payload_construct failed, error: %d", err);
		return;
	}

	message[sizeof(message) - 1] = '\0';

	tx_data.ptr = message;
	tx_data.len = strlen(message);

	LOG_INF("Shadow JSON len=%d", (int)tx_data.len);

	size_t dump_len = tx_data.len;
	if (dump_len > 64) dump_len = 64;
	LOG_HEXDUMP_INF(message, dump_len, "Shadow JSON preview");

	LOG_INF("Publishing Shadow Update:");
	LOG_INF("THING: %s", DEVICE_THING_NAME);
	LOG_INF("DATA : %s", message);

	err = aws_iot_send(&tx_data);
	if (err) {
		LOG_ERR("aws_iot_send (shadow update) failed, error: %d", err);
		return;
	}

	(void)k_work_reschedule(&shadow_update_work,
				K_SECONDS(CONFIG_AWS_IOT_PUBLICATION_INTERVAL_SECONDS));

	SHADOW_PUBLISHED = true;
}

/* ---------------- Connection handling ---------------- */

static void connect_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	int err;
	const struct aws_iot_config config = {
		.client_id = DEVICE_THING_NAME,
	};

	LOG_INF("Connecting to AWS IoT");

	err = aws_iot_connect(&config);

	if (err == -EAGAIN) {
		LOG_INF("Connection timed out, retry in %d seconds",
			CONFIG_AWS_IOT_CONNECTION_RETRY_TIMEOUT_SECONDS);

		(void)k_work_reschedule(&connect_work,
			K_SECONDS(CONFIG_AWS_IOT_CONNECTION_RETRY_TIMEOUT_SECONDS));
	} else if (err) {
		LOG_ERR("aws_iot_connect, error: %d", err);
		FATAL_ERROR();
	}
}

static void on_aws_iot_evt_connected(const struct aws_iot_evt *const evt)
{
	(void)k_work_cancel_delayable(&connect_work);

	if (evt->data.persistent_session) {
		LOG_WRN("Persistent session enabled; using previous subscriptions");
	}

#if defined(CONFIG_BOOTLOADER_MCUBOOT)
	boot_write_img_confirmed();
#endif

	/* ✅ Ensure we always fetch desired after reconnect */
	request_shadow_get();

	if (!device_sleep) {
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
	(void)aws_iot_disconnect();
	(void)k_work_cancel_delayable(&connect_work);

	if (evt->data.image & DFU_TARGET_IMAGE_TYPE_ANY_APPLICATION) {
		LOG_INF("Application FOTA done, rebooting");
		IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)));
	} else {
		LOG_WRN("Unexpected FOTA image type");
	}
}

void on_net_event_l4_connected(void)
{
	(void)k_work_reschedule(&connect_work, K_SECONDS(5));
}

void on_net_event_l4_disconnected(void)
{
	(void)k_work_cancel_delayable(&connect_work);
	(void)k_work_cancel_delayable(&shadow_update_work);
}

/* ---------------- Publish helper ---------------- */

int aws_iot_publish_topic(const char *topic, const char *payload, enum mqtt_qos qos)
{
	if (!topic || !payload) {
		LOG_ERR("Topic or payload is NULL");
		return -EINVAL;
	}

	struct aws_iot_topic_data topic_data = {
		.str = topic,
		.len = strlen(topic),
	};

	struct aws_iot_data msg = {
		.ptr = (char *)payload,
		.len = strlen(payload),
		.qos = qos,
		.topic = topic_data,
	};

	int err = aws_iot_send(&msg);
	if (err) {
		LOG_ERR("aws_iot_send failed: %d", err);
		return err;
	}

	LOG_INF("Publishing DATA :");
	LOG_WRN("TOPIC [ %s ]", topic);
	LOG_INF("%s", payload);
	return 0;
}

/* ---------------- Event handler ---------------- */

static void aws_iot_event_handler(const struct aws_iot_evt *const evt)
{
	switch (evt->type) {
	case AWS_IOT_EVT_CONNECTING:
		LOG_INF("AWS_IOT_EVT_CONNECTING");
		break;

	case AWS_IOT_EVT_CONNECTED:
		LOG_INF("AWS_IOT_EVT_CONNECTED");
		AWS_IOT_MQTT_CONNECTED = true;
		AWS_IOT_MQTT_DISCONNECTED = false;
		on_aws_iot_evt_connected(evt);
		break;

	case AWS_IOT_EVT_DISCONNECTED:
		LOG_INF("AWS_IOT_EVT_DISCONNECTED");
		AWS_IOT_MQTT_DISCONNECTED = true;
		AWS_IOT_MQTT_CONNECTED = false;
		on_aws_iot_evt_disconnected();
		break;

	case AWS_IOT_EVT_DATA_RECEIVED: {
		const char *t = evt->data.msg.topic.str;
		size_t tlen = evt->data.msg.topic.len;

		LOG_INF("AWS_IOT_EVT_DATA_RECEIVED:");
		LOG_INF("DATA : \"%.*s\"", evt->data.msg.len, evt->data.msg.ptr);
		LOG_WRN("TOPIC: \"%.*s\"", (int)tlen, t);

		if (tlen == strlen(topic_shadow_delta) &&
		    strncmp(t, topic_shadow_delta, tlen) == 0) {
			handle_shadow_delta_simple(evt->data.msg.ptr, evt->data.msg.len);
		} else if (tlen == strlen(topic_shadow_get_accepted) &&
			   strncmp(t, topic_shadow_get_accepted, tlen) == 0) {
			handle_shadow_get_accepted_simple(evt->data.msg.ptr, evt->data.msg.len);
		}

		break;
	}

	case AWS_IOT_EVT_PUBACK:
		LOG_INF("AWS_IOT_EVT_PUBACK, message ID: %d", evt->data.message_id);
		break;

	case AWS_IOT_EVT_PINGRESP:
		LOG_INF("AWS_IOT_EVT_PINGRESP");
		break;

	case AWS_IOT_EVT_FOTA_START:
		OTA_STARTED = true;
		LOG_INF("AWS_IOT_EVT_FOTA_START");
		break;

	case AWS_IOT_EVT_FOTA_DONE:
		LOG_INF("AWS_IOT_EVT_FOTA_DONE");
		OTA_STARTED = false;
		on_aws_iot_evt_fota_done(evt);
		break;

	case AWS_IOT_EVT_FOTA_DL_PROGRESS:
		LOG_INF("AWS_IOT_EVT_FOTA_DL_PROGRESS (%d%%)", evt->data.fota_progress);
		break;

	case AWS_IOT_EVT_ERROR:
		LOG_ERR("AWS_IOT_EVT_ERROR (%d)", evt->data.err);
		FATAL_ERROR();
		break;

	case AWS_IOT_EVT_FOTA_ERROR:
		OTA_STARTED = false;
		LOG_ERR("AWS_IOT_EVT_FOTA_ERROR");
		break;

	default:
		LOG_WRN("Unknown AWS IoT event type: %d", evt->type);
		break;
	}
}

/* ---------------- Public API ---------------- */

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
	if (err) {
		return err;
	}

	if (MQTT_INIT) {
		return 0;
	}

	err = aws_iot_init(aws_iot_event_handler);
	if (err) {
		LOG_ERR("aws_iot_init failed, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	err = app_topics_subscribe();
	if (err) {
		LOG_ERR("Topic subscribe setup failed, error: %d", err);
		FATAL_ERROR();
		return err;
	}

	MQTT_INIT = true;
	return 0;
}

int aws_iot_mqtt_disconnect(void)
{
	int err = aws_iot_disconnect();
	if (err) {
		LOG_ERR("Failed to disconnect AWS IoT MQTT: %d", err);
		return err;
	}

	LOG_WRN("AWS IoT MQTT Disconnected");
	return 0;
}
