#ifndef _AWS_IOT_MQTT_H
#define _AWS_IOT_MQTT_H

#include <hw_id.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <net/aws_iot.h>

#define FATAL_ERROR()                              \
	LOG_ERR("Fatal error! Rebooting the device."); \
	LOG_PANIC();                                   \
	IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)))

/* device-agnostic suffixes */
#define TOPIC_SUFFIX_TELEMETRY_PUB  "telemetry/publish"
#define TOPIC_SUFFIX_TELEMETRY_SUB  "telemetry/subscribe"

extern bool OTA_STARTED;
extern bool MQTT_INIT;
extern bool SHADOW_PUBLISHED;
extern bool AWS_IOT_FINISHED;
extern bool AWS_IOT_MQTT_CONNECTED;
extern bool AWS_IOT_MQTT_DISCONNECTED;

extern char DEVICE_THING_NAME[HW_ID_LEN];

void on_net_event_l4_connected(void);
void on_net_event_l4_disconnected(void);

void aws_iot_cancel_shadow_work(void);
int  aws_iot_client_init(void);
int  aws_iot_mqtt_disconnect(void);

int aws_iot_publish_topic(const char *topic,
			  const char *payload,
			  enum mqtt_qos qos);

const char *aws_iot_get_telemetry_pub_topic(void);

#endif
