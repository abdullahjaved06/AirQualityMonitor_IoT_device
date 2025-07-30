#include "scd41.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

static const struct device *dev = DEVICE_DT_GET_ONE(sensirion_scd41);

int scd41_device_check(void)
{
    if (!device_is_ready(dev)) {
        printf("SCD41 device not ready\n");
        return -ENODEV;
    }
    return 0;
}

float scd41_read_co2(void)
{
    struct sensor_value co2;
    int err;

    err = sensor_sample_fetch(dev);
    if (err) {
        printf("SCD41 fetch error: %d\n", err);
        return -1.0f;
    }

    err = sensor_channel_get(dev, SENSOR_CHAN_CO2, &co2);
    if (err) {
        printf("SCD41 CO₂ read error: %d\n", err);
        return -1.0f;
    }

    float co2_val = sensor_value_to_double(&co2);
    printf("CO₂: %.2f ppm\n", co2_val);

    return co2_val;
}

