// /*
//  * EPD Display Driver Header
//  * Portrait mode with rotation support
//  */

// #ifndef DISPLAY_H
// #define DISPLAY_H

// #include <stdbool.h>
// #include <stdint.h>

// /**
//  * Initialize the e-paper display
//  * @return 0 on success, negative error code on failure
//  */
// int epd_init(void);

// /**
//  * Clear the display buffer (white)
//  */
// void epd_clear(void);

// /**
//  * Refresh the display (send framebuffer to hardware)
//  */
// void epd_refresh(void);

// /**
//  * Draw the main UI with sensor data
//  * 
//  * @param co2_ppm       CO2 concentration in ppm
//  * @param temperature   Temperature in Celsius
//  * @param humidity      Relative humidity in percent
//  * @param battery_percent Battery level (0-100)
//  * @param charging      True if charging
//  */
// void epd_draw_ui(int co2_ppm, float temperature, float humidity,
//                  int battery_percent, bool charging);

// /**
//  * Get display dimensions (virtual/portrait)
//  */
// uint16_t epd_get_width(void);
// uint16_t epd_get_height(void);

// /**
//  * Check if display is initialized
//  */
// bool epd_is_initialized(void);

// #endif /* DISPLAY_H */