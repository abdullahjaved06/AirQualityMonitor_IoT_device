#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include "aws_iot_mqtt.h"
#include "json_payload.h"
#include "certificates.h"
#include "lte_manager.h"
#include "common.h"

#include "../components/scd41/scd41.h"
#include "../components/sht40/sht40.h"
#include "../components/leds/led.h"
#include "../components/npm1300/npm1300.h"
#include "../components/epd/epd_display.h"
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

extern const struct device *charger;   
extern volatile bool vbus_connected;
void peripherals_init(void);
float read_battery_voltage(void);
void on_power_event(power_event_t event);
 void event_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
 void epd_draw_ui(int co2_ppm, float temperature, float humidity,
                 int battery_percent, bool charging);
LOG_MODULE_REGISTER(MAIN);


#define ONE_MINUTE_MS 60000
#define TEN_MINUTES_MS 600000
#define TWO_MINUTES_MS 120000

uint64_t AWS_IOT_WAIT_TIME = 0;

float temp = 0.0f;
float hum = 0.0f;
float co2 = 0.0f;
float voltage=0.0f;
char power_source[16] = "BATTERY";  

bool high_priority_alert = false;
bool low_priority_alert = false;
int epd_init(void);
void epd_draw_ui(int co2_ppm, float temperature, float humidity,
                 int battery_percent, bool charging);
float get_battery_soc(void);
float get_battery_voltage(void);

bool device_sleep = false;

void publish_named_shadow_state(const char *thing_name, const char *shadow_name)
{
	if (!thing_name || !shadow_name)
	{
		LOG_ERR("Thing name or shadow name is NULL");
		return;
	}

	char topic[128];
	snprintf(topic, sizeof(topic),
			 "$aws/things/%s/shadow/name/%s/update",
			 thing_name, shadow_name);

	char payload[256];
	snprintf(payload, sizeof(payload),
			 "{"
			 "\"state\": {"
			 "  \"reported\": {"
			 "    \"temperature\": %.2f,"
			 "    \"humidity\": %.2f,"
			 "    \"co2\": %.2f"
			 "  }"
			 "}"
			 "}",
			 temp, hum, co2);

	int err = aws_iot_publish_topic(topic, payload, MQTT_QOS_1_AT_LEAST_ONCE);
	if (err)
	{
		LOG_ERR("Failed to publish named shadow: %d", err);
	}
}

int main(void)
{
	LOG_INF("Firmware Version: Alpha_0.2.");

	uint64_t device_sleep_time = k_uptime_get_32();
	LOG_INF("The AWS IoT MQTT started, version: %s\n\r", CONFIG_AWS_IOT_APP_VERSION);
	// write_device_certs_to_modem();    //writes certs in modem.
	LOG_INF("Initializing display...");
	int ret = epd_init();
	if (ret != 0) {
		LOG_ERR("Display init failed: %d", ret);
	} 
	else {
		k_msleep(100);  /* Small delay before first draw */
		epd_draw_ui(2222, 33.5f, 44.0f, 55, true);
	}
	peripherals_init();
	k_msleep(2000);
 // Register power event callback BEFORE enable_regulator()
    npm1300_register_power_callback(on_power_event);
	enable_regulator();
/* Initialize display */
k_msleep(2000);

	int err;

	// const char *topic = MY_CUSTOM_TOPIC_PUB;
	const char *topic = NULL;
	DEVICE_STATE = DEVICE_STATE_INIT;

	register_lte_lc_event_handler();
	

	while (1)
	{
		switch (DEVICE_STATE)
		{

		case DEVICE_STATE_INIT:
			LOG_INF("DEVICE STATE : DEVICE INIT\n\r");
			err = lte_net_mgmt_connect();
			if (err)
			{
				LOG_ERR("LTE Net mgmt Failed, error: %d", err);
				FATAL_ERROR();
			}
			err = aws_iot_client_init();
			if (err)
			{
				LOG_ERR("aws_iot_client_init, error: %d", err);
				FATAL_ERROR();
				return err;
			}
			topic = aws_iot_get_telemetry_pub_topic();
			if (!topic || topic[0] == '\0') {
				LOG_ERR("Telemetry topic not ready");
				FATAL_ERROR();
			}
			LOG_INF("Telemetry publish topic: %s", topic);

			DEVICE_STATE = DEVICE_STATE_LTE_CONNECT;
			break;

		case DEVICE_STATE_LTE_CONNECT:
			if (LTE_CONNECTED)
			{
				LOG_INF("DEVICE STATE : LTE CONNECT\n\r");
				int16_t rsrp;

			if (lte_read_rsrp_dbm(&rsrp)) {
				const char *q = rsrp_quality_label(rsrp);
				printk("RSRP=%d dBm (%s)\n", rsrp, q);

				// EPD draw example:
				// draw_text(0, 0, "LTE:");
				// draw_text(0, 16, "RSRP: -95 dBm");
				// draw_text(0, 32, "Quality: FAIR");
			} else {
				printk("RSRP read failed\n");
			}


				DEVICE_STATE = DEVICE_BATTERY_FUEL_GUAGE;
			}
			break;

		case DEVICE_BATTERY_FUEL_GUAGE:
		k_msleep(800);
			 // Only control power if lsout_feature is enabled.
			if (lsout_feature) {
				printk("Sensor Power save feature Enabled.\n\r");
				k_msleep(800);
				printk("Regulator Turned On.\n\r");
				enable_regulator();
				printk("Power On delay: %d \n\r",poweron_delay);
				k_msleep(poweron_delay);  // Use shadow-controlled delay

			} else { 
				printk("Sensor Power save feature Not Enabled.\n\r");

			}
				float soc = get_battery_soc();
				float voltage = get_battery_voltage();
			
				printk("Battery: %.1f%% (%.3fV)", (double)soc, (double)voltage);
    
			DEVICE_STATE = DEVICE_STATE_TEMP_HUM;
    		break;


		case DEVICE_STATE_TEMP_HUM:
		
			temp = sht4x_read_temperature();
		    epd_draw_ui(9999, 69.5f, 50.0f, 54, true);

			// Check temperature
			if (temp > temp_high_threshold || temp < temp_low_threshold)
			{
				high_priority_alert = true;
			}
			hum = sht4x_read_humidity();
			// Check humidity
			if (hum > hum_high_threshold || hum < hum_low_threshold)
			{
				high_priority_alert = true;
			}

				// Set LED mode accordingly
			if (high_priority_alert) {
				start_led_alert(LED_MODE_ALERT_RED);
			} else if (low_priority_alert) {
				start_led_alert(LED_MODE_ALERT_ORANGE);
			} else {
				stop_led_alert(); // No alerts
			}
			DEVICE_STATE = DEVICE_STATE_CO2;
			break;

		case DEVICE_STATE_CO2:
			co2 = scd41_read_co2();
			// Check CO2
			if (co2 > co2_high_threshold) {
				high_priority_alert = true;
			} else if (co2 > co2_medium_threshold) {
				low_priority_alert = true;
			}
			 // Only control power if lsout_feature is enabled.
			if (lsout_feature) {
				disable_regulator();
				printk("Regulator Turned OFF.");
			}
			DEVICE_STATE = DEVICE_STATE_POWER_SOURCE;
			break;
		case DEVICE_STATE_POWER_SOURCE:
			if (vbus_connected == true) {
			strcpy(power_source, "VBUS");
				LOG_INF("Power Source: VBUS");
			} else {
				strcpy(power_source, "BATTERY");
				LOG_INF("Power Source: BATTERY");
			}
			DEVICE_STATE = DEVICE_STATE_AWS_SEND_DATA;
			break;

		case DEVICE_STATE_AWS_SEND_DATA:
			if (!OTA_STARTED && AWS_IOT_MQTT_CONNECTED)
			{
				LOG_INF("DEVICE STATE : AWS SEND DATA\n\r");

				char payload[256];
				snprintf(payload, sizeof(payload),
					"{"
					"\"temperature\": %.2f,"
					"\"humidity\": %.2f,"
					"\"co2\": %.2f,"
					"\"voltage\": %.2f,"
					"\"charge(soc)\": %.2f,"
					"\"powersource\": \"%s\""
					"}",
					temp, hum, co2, voltage,soc, power_source);


				err = aws_iot_publish_topic(topic, payload, MQTT_QOS_0_AT_MOST_ONCE);
				if (err)
				{
					LOG_ERR("Failed to publish sensor data: %d", err);
				}

				k_msleep(6000);
				AWS_IOT_WAIT_TIME = k_uptime_get_32();

				if (OTA_STARTED)
				{
					DEVICE_STATE = DEVICE_STATE_OTA;
					break;
				}
				DEVICE_STATE = DEVICE_STATE_SLEEP;
			}
			break;

		case DEVICE_STATE_OTA:
			k_msleep(1000);
			break;

		case DEVICE_STATE_SLEEP:
			while (1)
			{
				if (SHADOW_PUBLISHED || k_uptime_get_32() - AWS_IOT_WAIT_TIME >= TWO_MINUTES_MS)
				{
					break;
				}
				k_msleep(500);
			}
			if (OTA_STARTED)
			{
				DEVICE_STATE = DEVICE_STATE_OTA;
				break;
			}
			device_sleep = true;
			aws_iot_cancel_shadow_work();

#if CONFIG_AWS_IOT_USE_LTE_POWER_OFF
			aws_iot_mqtt_disconnect();
			lte_net_mgmt_disconnect();
			LOG_INF("DEVICE STATE : SLEEP LTE Power off TIME : [ %d ]\n\r", device_sleep_time_minutes);
#elif CONFIG_AWS_IOT_USE_EDRX
			LOG_INF("DEVICE STATE : SLEEP EDRX Request TIME [%d]\n\r", device_sleep_time_minutes);
			device_sleep_request_edrx(device_sleep_time_minutes);
#endif
			DEVICE_STATE = DEVICE_STATE_IDLE;
			break;

		case DEVICE_STATE_IDLE:
			if (k_uptime_get_32() - device_sleep_time >= device_sleep_time_minutes)
			{
				LOG_WRN("Time passed after sleep %lld\n\r", device_sleep_time);
				device_sleep = false;
				device_sleep_time = k_uptime_get_32();
#if CONFIG_AWS_IOT_USE_EDRX
				DEVICE_STATE = DEVICE_BATTERY_FUEL_GUAGE;
#elif CONFIG_AWS_IOT_USE_LTE_POWER_OFF
				DEVICE_STATE = DEVICE_STATE_INIT;
#endif
			}
			break;

		default:
			break;
		}
		k_msleep(200);
	}
	return 0;
}

void peripherals_init(void)
{
	if (scd41_device_check() == 0)
	{
		LOG_INF("SCD41 ready.\n");
	}
	else
	{
		LOG_ERR("SCD41 not ready!");
	}
	if (sht4x_device_check() == 0)
	{
		LOG_INF("SHT40 ready.\n");
	}
	else
	{
		LOG_ERR("SHT40 not ready!");
	}

	leds_init();
}


float read_battery_voltage(void)
{
    struct sensor_value val;
    float voltage;

    if (!device_is_ready(charger)) {
        printk("Charger not ready!\n");
        return -1.0f;
    }

    /* Trigger fresh measurement */
    if (sensor_sample_fetch(charger) < 0) {
        printk("Sensor fetch failed\n");
        return -1.0f;
    }

    /* Read voltage */
    if (sensor_channel_get(charger, SENSOR_CHAN_GAUGE_VOLTAGE, &val) < 0) {
        printk("Voltage read failed\n");
        return -1.0f;
    }

    voltage = (float)val.val1 + ((float)val.val2 / 1000000.0f);
    return voltage;
}

// NEW: Power event handler
void on_power_event(power_event_t event)
{
    // Only publish if connected to AWS
    if (!AWS_IOT_MQTT_CONNECTED) {
        LOG_WRN("AWS not connected, skipping power event publish");
        return;
    }

    const char *topic = aws_iot_get_telemetry_pub_topic();
    if (!topic) {
        LOG_ERR("Topic not ready");
        return;
    }

    char payload[128];
    const char *event_str;
    
    switch (event) {
        case POWER_EVENT_USB_CONNECTED:
            event_str = "USB_CONNECTED";
            LOG_INF("Publishing USB connected event");
            break;
        case POWER_EVENT_USB_DISCONNECTED:
            event_str = "USB_DISCONNECTED";
            LOG_INF("Publishing USB disconnected event");
            break;
        default:
            return;
    }

    snprintf(payload, sizeof(payload),
             "{"
             "\"event\": \"%s\","
             "\"timestamp\": %lld,"
             "\"vbus_connected\": %s"
             "}",
             event_str,
             k_uptime_get(),
             vbus_connected ? "true" : "false");

    int err = aws_iot_publish_topic(topic, payload, MQTT_QOS_1_AT_LEAST_ONCE);
    if (err) {
        LOG_ERR("Failed to publish power event: %d", err);
    } else {
        LOG_INF("Power event published: %s", event_str);
    }
}