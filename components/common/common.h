#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>
#include <stdbool.h>

extern uint32_t device_sleep_time_minutes;

extern bool sensor_co2_enable;

typedef enum
{
    DEVICE_STATE_INIT,
    DEVICE_STATE_LTE_CONNECT,
    DEVICE_STATE_AWS_SEND_DATA,
    DEVICE_STATE_SLEEP,
    DEVICE_STATE_IDLE,
    DEVICE_STATE_OTA,
} SystemState;

extern SystemState DEVICE_STATE;

#endif