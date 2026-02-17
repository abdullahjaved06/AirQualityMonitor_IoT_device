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

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm1300_charger.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <nrf_fuel_gauge.h>

int fuel_gauge_init(const struct device *charger);


static const struct device *pmic = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_pmic));
static const struct device *leds = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_leds));
static const struct device *regulators = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_regulators));
// static const struct device *ldsw1 = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_ldo1));
static const struct device *ldsw2 = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_ldo2));
// const struct device *npm1300_gpio = DEVICE_DT_GET(DT_NODELABEL(npm1300_ek_gpio));

 void event_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
bool configure_events(void);
void enable_regulator();
void disable_regulator();
float get_battery_soc(void);
float get_battery_voltage(void);
int fuel_gauge_init(const struct device *charger);
int fuel_gauge_update(const struct device *charger, bool vbus_connected);
// float read_battery_voltage(void);

//  Power event types
typedef enum {
    POWER_EVENT_NONE,
    POWER_EVENT_USB_CONNECTED,
    POWER_EVENT_USB_DISCONNECTED
} power_event_t;

//  Register callback for power events
typedef void (*power_event_callback_t)(power_event_t event);
void npm1300_register_power_callback(power_event_callback_t callback);

#endif /* NPM1300_H */
