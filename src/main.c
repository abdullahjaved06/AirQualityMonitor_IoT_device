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

void peripherals_init(void);
LOG_MODULE_REGISTER(MAIN);

#define ONE_MINUTE_MS 60000
#define TEN_MINUTES_MS 600000
#define TWO_MINUTES_MS 120000

uint64_t AWS_IOT_WAIT_TIME = 0;

float temp = 0.0f;
float hum = 0.0f;
float co2 = 0.0f;

bool device_sleep = false;

void publish_named_shadow_state(const char *thing_name, const char *shadow_name)
{
	if (!thing_name || !shadow_name) {
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
	if (err) {
		LOG_ERR("Failed to publish named shadow: %d", err);
	}
}

int main(void)
{
	uint64_t device_sleep_time = k_uptime_get_32();
	LOG_INF("The AWS IoT MQTT started, version: %s\n\r", CONFIG_AWS_IOT_APP_VERSION);
	peripherals_init();
	int err;

	const char *topic = MY_CUSTOM_TOPIC_PUB;
	DEVICE_STATE = DEVICE_STATE_INIT;

	register_lte_lc_event_handler();

	while (1) {
		switch (DEVICE_STATE) {

		case DEVICE_STATE_TEMP_HUM:
			temp = sht4x_read_temperature();
			hum = sht4x_read_humidity();
			break;

		case DEVICE_STATE_CO2:
			co2 = scd41_read_co2();
			break;

		case DEVICE_STATE_INIT:
			LOG_INF("DEVICE STATE : DEVICE INIT\n\r");
			err = lte_net_mgmt_connect();
			if (err) {
				LOG_ERR("LTE Net mgmt Failed, error: %d", err);
				FATAL_ERROR();
			}
			err = aws_iot_client_init();
			if (err) {
				LOG_ERR("aws_iot_client_init, error: %d", err);
				FATAL_ERROR();
				return err;
			}
			DEVICE_STATE = DEVICE_STATE_LTE_CONNECT;
			break;

		case DEVICE_STATE_LTE_CONNECT:
			if (LTE_CONNECTED) {
				LOG_INF("DEVICE STATE : LTE CONNECT\n\r");
				DEVICE_STATE = DEVICE_STATE_AWS_SEND_DATA;
			}
			break;

		case DEVICE_STATE_AWS_SEND_DATA:
			if (!OTA_STARTED && AWS_IOT_MQTT_CONNECTED) {
				LOG_INF("DEVICE STATE : AWS SEND DATA\n\r");

				char payload[256];
				snprintf(payload, sizeof(payload),
						 "{"
						 "\"temperature\": %.2f,"
						 "\"humidity\": %.2f,"
						 "\"co2\": %.2f"
						 "}",
						 temp, hum, co2);

				err = aws_iot_publish_topic(topic, payload, MQTT_QOS_0_AT_MOST_ONCE);
				if (err) {
					LOG_ERR("Failed to publish sensor data: %d", err);
				}

				k_msleep(6000);
				AWS_IOT_WAIT_TIME = k_uptime_get_32();

				if (OTA_STARTED) {
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
			while (1) {
				if (SHADOW_PUBLISHED || k_uptime_get_32() - AWS_IOT_WAIT_TIME >= TWO_MINUTES_MS) {
					break;
				}
				k_msleep(500);
			}
			if (OTA_STARTED) {
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
			if (k_uptime_get_32() - device_sleep_time >= device_sleep_time_minutes) {
				LOG_WRN("Time passed after sleep %lld\n\r", device_sleep_time);
				device_sleep = false;
				device_sleep_time = k_uptime_get_32();
#if CONFIG_AWS_IOT_USE_EDRX
				DEVICE_STATE = DEVICE_STATE_AWS_SEND_DATA;
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
	if (scd41_device_check() == 0) {
		LOG_INF("SCD41 ready.\n");
	} else {
		LOG_ERR("SCD41 not ready!");
	}
	if (sht4x_device_check() == 0) {
		LOG_INF("SHT40 ready.\n");
	} else {
		LOG_ERR("SHT40 not ready!");
	}
}
