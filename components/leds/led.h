#ifndef _LEDS_H
#define _LEDS_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

static const struct gpio_dt_spec green_led = GPIO_DT_SPEC_GET(DT_NODELABEL(green_led), gpios);
static const struct gpio_dt_spec blue_led  = GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led), gpios);
static const struct gpio_dt_spec red_led   = GPIO_DT_SPEC_GET(DT_NODELABEL(red_led), gpios);

void leds_init(void);
void flash_green_timer_handler(struct k_timer *timer_id);
void start_flashing_green_led(void);
void stop_flashing_green_led(void);
void stop_solid_leds(void);
void update_led_from_lte_status(void);

enum led_mode_t {
    LED_MODE_NORMAL,
    LED_MODE_ALERT_RED,
    LED_MODE_ALERT_ORANGE,
};

static enum led_mode_t current_led_mode = LED_MODE_NORMAL;
static bool show_alert_color = false;

#define ALERT_BLINK_INTERVAL K_SECONDS(5)

 void alert_led_timer_handler(struct k_timer *timer_id);





#endif // _LEDS_H



