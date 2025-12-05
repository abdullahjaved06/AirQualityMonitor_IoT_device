#ifndef NPM1300_H
#define NPM1300_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/mfd/npm1300.h>
#include <zephyr/dt-bindings/gpio/nordic-npm1300-gpio.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/sensor.h>

static const struct device *pmic = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_pmic));
static const struct device *leds = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_leds));
static const struct device *regulators = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_regulators));
// static const struct device *ldsw1 = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_ldo1));
static const struct device *ldsw2 = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_ldo2));
static const struct device *charger = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_charger));
// const struct device *npm1300_gpio = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_gpio));

static void event_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
bool configure_events(void);
void enable_regulator();



#endif /* NPM1300_H */
