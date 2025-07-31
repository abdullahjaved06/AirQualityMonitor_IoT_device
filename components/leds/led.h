#ifndef _LEDS_H
#define _LEDS_H

#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec green_led = GPIO_DT_SPEC_GET(DT_NODELABEL(green_led), gpios);
static const struct gpio_dt_spec blue_led  = GPIO_DT_SPEC_GET(DT_NODELABEL(blue_led), gpios);
static const struct gpio_dt_spec red_led   = GPIO_DT_SPEC_GET(DT_NODELABEL(red_led), gpios);

void leds_init(void);

#endif // _LEDS_H



