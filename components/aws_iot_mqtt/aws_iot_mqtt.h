#ifndef _AWS_IOT_MQTT_H
#define _AWS_IOT_MQTT_H

#include <hw_id.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <net/aws_iot.h>

/* Macro called upon a fatal error, reboots the device. */
#define FATAL_ERROR()                              \
    LOG_ERR("Fatal error! Rebooting the device."); \
    LOG_PANIC();                                   \
    IF_ENABLED(CONFIG_REBOOT, (sys_reboot(0)))

#define MY_CUSTOM_TOPIC_1 "aws/things/359746167121867/sensorData/subscribe"
#define MY_CUSTOM_TOPIC_2 "aws/things/359746167121867/telemetry/subscribe"
#define MY_CUSTOM_TOPIC_PUB "aws/things/359746167121867/telemetry/publish"

#define SENSOR_SHADOW_TOPIC_DELTA "$aws/things/359746167121867/shadow/name/sensor/update/delta"
#define SENSOR_SHADOW_TOPIC_UPDATE "$aws/things/359746167121867/shadow/name/sensor/update"
#define SENSOR_SHADOW_TOPIC_REJECTED "$aws/things/359746167121867/shadow/name/sensor/update/rejected"
#define SENSOR_SHADOW_TOPIC_ACCEPTED "$aws/things/359746167121867/shadow/name/sensor/update/accepted"

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
int aws_iot_client_init(void);
int aws_iot_mqtt_disconnect(void);
int aws_iot_publish_topic(const char *topic,
                          const char *payload,
                          enum mqtt_qos qos);

#endif