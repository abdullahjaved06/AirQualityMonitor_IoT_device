#include "led.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(leds, LOG_LEVEL_INF);

void leds_init(void)
{
	const struct gpio_dt_spec *leds[] = { &green_led, &blue_led, &red_led };

	for (int i = 0; i < ARRAY_SIZE(leds); i++) {
		if (!device_is_ready(leds[i]->port)) {
			LOG_ERR("LED %d port not ready", i);
			continue;
		}
		if (gpio_pin_configure_dt(leds[i], GPIO_OUTPUT_INACTIVE) != 0) {
			LOG_ERR("Failed to configure LED %d", i);
		}
	}
    // gpio_pin_set_dt(&green_led, 0);
	// gpio_pin_set_dt(&blue_led, 0);
	// gpio_pin_set_dt(&red_led, 0);
}
