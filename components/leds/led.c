#include "led.h"

#include <zephyr/logging/log.h>

extern bool LTE_CONNECTED; 


LOG_MODULE_REGISTER(leds, LOG_LEVEL_INF);

 struct k_timer green_led_flash_timer;
static bool green_led_on = false;
K_TIMER_DEFINE(green_led_flash_timer, flash_green_timer_handler, NULL);
K_TIMER_DEFINE(alert_led_timer, alert_led_timer_handler, NULL);


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

void flash_green_timer_handler(struct k_timer *timer_id)
{
    green_led_on = !green_led_on;
    gpio_pin_set_dt(&green_led, green_led_on);
}

void start_flashing_green_led(void)
{
    // Ensure LED starts in known state
    green_led_on = false;
    gpio_pin_set_dt(&green_led, 0);

    // Start 1Hz (1000 ms) timer
    k_timer_start(&green_led_flash_timer, K_NO_WAIT, K_SECONDS(1));
}

void stop_flashing_green_led(void)
{
    k_timer_stop(&green_led_flash_timer);
    green_led_on = false;
    gpio_pin_set_dt(&green_led, 0);
}


void stop_solid_leds(void)
{
    gpio_pin_set_dt(&green_led, 0);
    gpio_pin_set_dt(&blue_led, 0);
}

 void alert_led_timer_handler(struct k_timer *timer_id)
{
    show_alert_color = !show_alert_color;

    if (current_led_mode == LED_MODE_ALERT_RED && show_alert_color) {
        // RED
        gpio_pin_set_dt(&red_led, 1);
        gpio_pin_set_dt(&green_led, 0);
        gpio_pin_set_dt(&blue_led, 0);
    } else if (current_led_mode == LED_MODE_ALERT_ORANGE && show_alert_color) {
        // ORANGE (Red + Green)
        gpio_pin_set_dt(&red_led, 1);
        gpio_pin_set_dt(&green_led, 1);
        gpio_pin_set_dt(&blue_led, 0);
    } else {
              // Show LTE status (green/blue/flashing)
        gpio_pin_set_dt(&red_led, 0);    // Turn off red from alert state
        update_led_from_lte_status();
    }
}

void start_led_alert(enum led_mode_t mode)
{
    current_led_mode = mode;
    show_alert_color = false;
    k_timer_start(&alert_led_timer, K_NO_WAIT, ALERT_BLINK_INTERVAL);
}

void stop_led_alert(void)
{
    k_timer_stop(&alert_led_timer);
    current_led_mode = LED_MODE_NORMAL;
    show_alert_color = false;
    update_led_from_lte_status();
}

void update_led_from_lte_status(void)
{
    if (LTE_CONNECTED) {
        // Device is connected
        stop_flashing_green_led();
        gpio_pin_set_dt(&green_led, 1);  // Solid green
        gpio_pin_set_dt(&blue_led, 0);
    } else {
        // Device is not fully connected – check what LED is currently active
        // If flashing is already active, keep it (do nothing)
        // Otherwise, assume disconnected = solid blue
    if (k_timer_status_get(&green_led_flash_timer) == 0) {
            gpio_pin_set_dt(&green_led, 0);
            gpio_pin_set_dt(&blue_led, 1);  // Solid blue
        }
    }
}
