#ifndef EPD_DISPLAY_H
#define EPD_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

int epd_init(void);
void epd_clear(void);
void epd_refresh(void);

/* Updated function with signal_rsrp parameter */
void epd_draw_ui(int co2_ppm, float temperature, float humidity,
                 int battery_percent, bool charging, int signal_rsrp);

uint16_t epd_get_width(void);
uint16_t epd_get_height(void);
bool epd_is_initialized(void);

#endif