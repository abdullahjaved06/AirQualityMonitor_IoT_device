#include "npm1300.h"

    LOG_MODULE_REGISTER(NPM1300);

#define FAST_FLASH_MS	100
#define SLOW_FLASH_MS	500
#define PRESS_SHORT_MS	1000
#define PRESS_MEDIUM_MS 5000

static volatile int flash_time_ms = SLOW_FLASH_MS;
static volatile bool vbus_connected;


static void event_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	static int press_t;

	if (pins & BIT(NPM1300_EVENT_SHIPHOLD_PRESS)) {
		press_t = k_uptime_get();
	}

	if (pins & BIT(NPM1300_EVENT_SHIPHOLD_RELEASE)) {
		press_t = k_uptime_get() - press_t;

		if (press_t < PRESS_SHORT_MS) {
			LOG_INF("Short press\n");
			if (!regulator_is_enabled(ldsw2)) {
				regulator_enable(ldsw2);
			}
			flash_time_ms = FAST_FLASH_MS;
		} else if (press_t < PRESS_MEDIUM_MS) {
			LOG_INF("Medium press\n");
			if (regulator_is_enabled(ldsw2)) {
				regulator_disable(ldsw2);
			}
			flash_time_ms = SLOW_FLASH_MS;
		} else {
			LOG_INF("Long press\n");
			if (vbus_connected) {
				LOG_INF("Ship mode entry not possible with USB connected\n");
			} else {
				regulator_parent_ship_mode(regulators);
			}
		}
	}

	if (pins & BIT(NPM1300_EVENT_VBUS_DETECTED)) {
		LOG_INF("Vbus connected\n");
		vbus_connected = true;
	}

	if (pins & BIT(NPM1300_EVENT_VBUS_REMOVED)) {
		LOG_INF("Vbus removed\n");
		vbus_connected = false;
	}
}

bool configure_events(void)
{
	if (!device_is_ready(pmic)) {
		LOG_INF("Pmic device not ready.\n");
		return false;
	}

	if (!device_is_ready(regulators)) {
		LOG_INF("Regulator device not ready.\n");
		return false;
	}

	if (!device_is_ready(ldsw2)) {
		LOG_INF("Load switch 2 device not ready.\n");
		return false;
	}

	if (!device_is_ready(charger)) {
		LOG_INF("Charger device not ready.\n");
		return false;
	}

// 	err = gpio_pin_configure(npm1300_gpio, 1, NPM1300_GPIO_PWRLOSSWARN_ON);
// if (err) {
// 	LOG_INF("NPM GPIO PIN CONFIG ERROR gpio_pin_configure=%d\n", err);
// 	return false;
// }

	static struct gpio_callback event_cb;

	gpio_init_callback(&event_cb, event_callback,
			   BIT(NPM1300_EVENT_SHIPHOLD_PRESS) | BIT(NPM1300_EVENT_SHIPHOLD_RELEASE) |
				   BIT(NPM1300_EVENT_VBUS_DETECTED) |
				   BIT(NPM1300_EVENT_VBUS_REMOVED));

	mfd_npm1300_add_callback(pmic, &event_cb);

	/* Initialise vbus detection status */
	struct sensor_value val;
	int ret = sensor_attr_get(charger, SENSOR_CHAN_CURRENT, SENSOR_ATTR_UPPER_THRESH, &val);

	if (ret < 0) {
		return false;
	}

	vbus_connected = (val.val1 != 0) || (val.val2 != 0);

	return true;
}

void enable_regulator()
{
    	if (!configure_events()) {
		LOG_INF("Error: could not configure events\n");
		return 0;
	}

	LOG_INF("PMIC device ok\n");
	regulator_enable(ldsw2);
}

