#include "sht40.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

static const struct device *dev = DEVICE_DT_GET_ONE(sensirion_sht4x);

int sht4x_device_check(void)
{
    if (!device_is_ready(dev)) {
        printf("SHT4x device not ready\n");
        return -ENODEV;
    }
    return 0;
}

float sht4x_read_temperature(void)
{
    struct sensor_value temp;
    if (sensor_sample_fetch(dev) != 0 ||
        sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) != 0) {
        return -1.0f;
    }
    return sensor_value_to_double(&temp);
}

float sht4x_read_humidity(void)
{
    struct sensor_value hum;
    if (sensor_sample_fetch(dev) != 0 ||
        sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &hum) != 0) {
        return -1.0f;
    }
    return sensor_value_to_double(&hum);
}

