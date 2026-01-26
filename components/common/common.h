#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>
#include <stdbool.h>

extern uint32_t device_sleep_time_minutes;

extern bool sensor_co2_enable;
extern bool lsout_feature;
extern uint32_t poweron_delay;


typedef enum
{
    DEVICE_STATE_INIT,
    DEVICE_STATE_LTE_CONNECT,
    DEVICE_STATE_AWS_SEND_DATA,
    DEVICE_STATE_SLEEP,
    DEVICE_STATE_IDLE,
    DEVICE_STATE_OTA,
    DEVICE_STATE_TEMP_HUM,
    DEVICE_STATE_CO2,
    DEVICE_BATTERY_FUEL_GUAGE,
    DEVICE_STATE_POWER_SOURCE,
} SystemState;

extern SystemState DEVICE_STATE;

#endif