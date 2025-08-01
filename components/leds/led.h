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






#endif // _LEDS_H



