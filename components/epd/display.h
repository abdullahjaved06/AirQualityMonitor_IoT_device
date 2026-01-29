/*
 * Simple EPD Display Driver - No LVGL
 * Header file
 */

#ifndef EPD_SIMPLE_H
#define EPD_SIMPLE_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Initialize the EPD display
 * @return 0 on success, negative error code on failure
 */
int epd_init(void);

/**
 * Clear the screen (fill with white)
 */
void epd_clear(void);

/**
 * Refresh the display (write framebuffer to screen)
 */
void epd_refresh(void);

/**
 * Draw the complete UI with sensor values
 * @param co2_ppm CO2 reading in ppm
 * @param temperature Temperature in Celsius
 * @param humidity Relative humidity percentage
 * @param battery_percent Battery level 0-100
 * @param charging True if charging
 */
void epd_draw_ui(int co2_ppm, float temperature, float humidity,
                 int battery_percent, bool charging);

/**
 * Get display width
 */
uint16_t epd_get_width(void);

/**
 * Get display height
 */
uint16_t epd_get_height(void);

#endif /* EPD_SIMPLE_H */