// /*
//  * EPD Display Driver
//  * 
//  * Numbers: Arial 28x36 (good!)
//  * Labels: Pixel font 8x12 (fixed % and m)
//  */

// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/display.h>
// #include <zephyr/logging/log.h>
// #include <string.h>
// #include <stdio.h>

// #include "epd_display.h"
// #include "../../fonts/pixel_font.h"

// LOG_MODULE_REGISTER(epd_display, LOG_LEVEL_DBG);

// /* Arial large number font */
// #define LARGE_FONT_WIDTH   28
// #define LARGE_FONT_HEIGHT  36
// #define LARGE_FONT_FIRST   46
// #define LARGE_FONT_LAST    57

// static const uint8_t cfb_font_arial_large[] = {
// #include "../../fonts/arial_large_data.inc"
// };

// /* Static framebuffer - sized for 250x122 display */
// /* fb_size = width * ((height + 7) / 8) = 250 * 16 = 4000 bytes */
// #define FB_STATIC_SIZE 4096
// static uint8_t framebuffer_static[FB_STATIC_SIZE];

// /* Display state */
// static const struct device *display_dev;
// static struct display_capabilities caps;
// static uint8_t *framebuffer = framebuffer_static;
// static size_t fb_size;
// static uint16_t phys_width, phys_height;
// static uint16_t virt_width, virt_height;
// static uint16_t tile_rows;
// static bool initialized = false;

// #define MARGIN 5

// /* Drawing primitives */
// static void set_pixel(uint16_t vx, uint16_t vy, bool black)
// {
//     if (vx >= virt_width || vy >= virt_height) return;
//     uint16_t px = vy;
//     uint16_t py = phys_height - 1 - vx;
//     if (px >= phys_width || py >= phys_height) return;
//     uint16_t tile_row = py / 8;
//     uint8_t bit = 7 - (py % 8);
//     size_t idx = tile_row * phys_width + px;
//     if (idx >= fb_size) return;
//     if (black) framebuffer[idx] &= ~(1 << bit);
//     else framebuffer[idx] |= (1 << bit);
// }

// static void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool black)
// {
//     for (uint16_t py = y; py < y + h && py < virt_height; py++)
//         for (uint16_t px = x; px < x + w && px < virt_width; px++)
//             set_pixel(px, py, black);
// }

// static void draw_hline(uint16_t x, uint16_t y, uint16_t len, bool black)
// {
//     for (uint16_t i = 0; i < len; i++) set_pixel(x + i, y, black);
// }

// static void clear_screen(void) { memset(framebuffer, 0xFF, fb_size); }

// /* Large number font (Arial) */
// static int draw_vpacked_char(uint16_t x, uint16_t y, const uint8_t *font_data,
//                              int font_width, int font_height,
//                              int first_char, int last_char, char c)
// {
//     if (c < first_char || c > last_char) return font_width / 2;
//     int char_idx = c - first_char;
//     int byte_rows = (font_height + 7) / 8;
//     int bytes_per_char = font_width * byte_rows;
//     const uint8_t *glyph = &font_data[char_idx * bytes_per_char];
    
//     for (int col = 0; col < font_width; col++) {
//         for (int byte_row = 0; byte_row < byte_rows; byte_row++) {
//             uint8_t byte_val = glyph[byte_row * font_width + col];
//             for (int bit = 0; bit < 8; bit++) {
//                 int py = byte_row * 8 + bit;
//                 if (py >= font_height) break;
//                 if (byte_val & (1 << bit)) set_pixel(x + col, y + py, true);
//             }
//         }
//     }
//     return font_width;
// }

// static int draw_large_char(uint16_t x, uint16_t y, char c)
// {
//     if (c == '.') {
//         fill_rect(x + 4, y + LARGE_FONT_HEIGHT - 7, 5, 5, true);
//         return 14;
//     }
//     draw_vpacked_char(x, y, cfb_font_arial_large, LARGE_FONT_WIDTH, LARGE_FONT_HEIGHT,
//                       LARGE_FONT_FIRST, LARGE_FONT_LAST, c);
//     return 26;
// }

// static int draw_large_string(uint16_t x, uint16_t y, const char *str)
// {
//     int start_x = x;
//     while (*str) { x += draw_large_char(x, y, *str); str++; }
//     return x - start_x;
// }

// static int get_large_string_width(const char *str)
// {
//     int width = 0;
//     while (*str) { width += (*str == '.') ? 14 : 26; str++; }
//     return width;
// }

// /* Small label font (pixel) */
// static int draw_small_char(uint16_t x, uint16_t y, char c)
// {
//     int idx = pixel_font_index(c);
//     const uint8_t *glyph = pixel_font_data[idx];
    
//     for (int row = 0; row < PIXEL_FONT_HEIGHT; row++) {
//         uint8_t bits = glyph[row];
//         for (int col = 0; col < PIXEL_FONT_WIDTH; col++) {
//             if (bits & (0x80 >> col)) set_pixel(x + col, y + row, true);
//         }
//     }
    
//     switch(c) {
//         case ' ': return 6;
//         case '(': case ')': return 6;
//         case '%': return 9;
//         case 'm': return 9;
//         default: return 9;
//     }
// }

// static int draw_small_string(uint16_t x, uint16_t y, const char *str)
// {
//     int start_x = x;
//     while (*str) { x += draw_small_char(x, y, *str); str++; }
//     return x - start_x;
// }

// /* Display API */
// static void refresh_display(void)
// {
//     LOG_DBG("EPD: Refreshing display...");
//     struct display_buffer_descriptor desc = {
//         .buf_size = fb_size, 
//         .width = phys_width, 
//         .height = phys_height, 
//         .pitch = phys_width,
//     };
//     int ret = display_write(display_dev, 0, 0, &desc, framebuffer);
//     if (ret != 0) {
//         LOG_ERR("EPD: display_write failed: %d", ret);
//     } else {
//         LOG_DBG("EPD: Write complete, waiting for e-paper...");
//         k_msleep(2000);  /* Wait for e-paper to physically update */
//         LOG_DBG("EPD: Refresh complete");
//     }
// }

// int epd_init(void)
// {
//     // ... existing code up to display_blanking_off ...
    
//     display_blanking_off(display_dev);
//     LOG_DBG("EPD: Blanking off");
    
//     /* Test: Do a blank refresh and wait */
//     LOG_INF("EPD: Doing initial test refresh...");
//     struct display_buffer_descriptor desc = {
//         .buf_size = fb_size,
//         .width = phys_width,
//         .height = phys_height,
//         .pitch = phys_width,
//     };
//     int ret = display_write(display_dev, 0, 0, &desc, framebuffer);
//     LOG_INF("EPD: display_write returned %d", ret);
    
//     /* Force wait for e-paper to finish */
//     LOG_INF("EPD: Waiting 3 seconds for e-paper refresh...");
//     k_msleep(3000);
//     LOG_INF("EPD: Wait complete");
    
//     initialized = true;
//     LOG_INF("EPD: Init complete");
//     return 0;
// }

// void epd_clear(void) 
// { 
//     if (initialized) {
//         clear_screen();
//         LOG_DBG("EPD: Buffer cleared");
//     }
// }

// void epd_refresh(void) 
// { 
//     if (initialized) {
//         refresh_display();
//     }
// }

// void epd_draw_ui(int co2_ppm, float temperature, float humidity,
//                  int battery_percent, bool charging)
// {
//     if (!initialized) {
//         LOG_WRN("EPD: draw_ui called but not initialized!");
//         return;
//     }
    
//     LOG_DBG("EPD: Drawing UI - CO2=%d, T=%.1f, H=%.1f, Batt=%d%%", 
//             co2_ppm, (double)temperature, (double)humidity, battery_percent);
    
//     char buf[16];
//     int content_width = virt_width - (2 * MARGIN);
//     clear_screen();
    
//     int header_h = 20;
//     int section_h = (virt_height - header_h) / 3;
//     int label_y = 5;
//     int number_y = 28;
    
//     /* Header */
//     int batt_x = MARGIN, batt_y = 5;
//     for (int i = 0; i <= 18; i++) { set_pixel(batt_x + i, batt_y, true); set_pixel(batt_x + i, batt_y + 10, true); }
//     for (int i = 0; i <= 10; i++) { set_pixel(batt_x, batt_y + i, true); set_pixel(batt_x + 18, batt_y + i, true); }
//     fill_rect(batt_x + 19, batt_y + 3, 3, 5, true);
//     int fill = (15 * battery_percent) / 100;
//     if (fill > 0) fill_rect(batt_x + 2, batt_y + 2, fill, 7, true);
//     int sig_x = batt_x + 28;
//     int bar_h[] = {3, 5, 7, 10};
//     for (int i = 0; i < 4; i++) fill_rect(sig_x + i * 5, batt_y + (10 - bar_h[i]), 3, bar_h[i], true);
//     if (charging) {
//         int chg_x = virt_width - MARGIN - 14;
//         fill_rect(chg_x + 4, batt_y, 4, 3, true);
//         fill_rect(chg_x + 2, batt_y + 3, 6, 2, true);
//         fill_rect(chg_x + 4, batt_y + 5, 4, 3, true);
//         fill_rect(chg_x + 5, batt_y + 8, 2, 3, true);
//     }
//     draw_hline(MARGIN, header_h, content_width, true);
    
//     /* CO2 */
//     int sec_y = header_h;
//     draw_small_string(MARGIN, sec_y + label_y, "CO2 (ppm)");
//     snprintf(buf, sizeof(buf), "%d", co2_ppm);
//     int w = get_large_string_width(buf);
//     draw_large_string((virt_width - w) / 2, sec_y + number_y, buf);
//     draw_hline(MARGIN, header_h + section_h, content_width, true);
    
//     /* Humidity */
//     sec_y = header_h + section_h;
//     draw_small_string(MARGIN, sec_y + label_y, "HUM (%)");
//     snprintf(buf, sizeof(buf), "%d", (int)humidity);
//     w = get_large_string_width(buf);
//     draw_large_string((virt_width - w) / 2, sec_y + number_y, buf);
//     draw_hline(MARGIN, header_h + section_h * 2, content_width, true);
    
//     /* Temperature */
//     sec_y = header_h + section_h * 2;
//     draw_small_string(MARGIN, sec_y + label_y, "TEMP (C)");
//     int t_int = (int)temperature;
//     int t_dec = (int)((temperature - t_int) * 10);
//     if (t_dec < 0) t_dec = -t_dec;
//     snprintf(buf, sizeof(buf), "%d.%d", t_int, t_dec);
//     w = get_large_string_width(buf);
//     draw_large_string((virt_width - w) / 2, sec_y + number_y, buf);
    
//     LOG_DBG("EPD: UI drawn, refreshing...");
//     refresh_display();
//     LOG_INF("EPD: UI update complete");
// }

// uint16_t epd_get_width(void) { return virt_width; }
// uint16_t epd_get_height(void) { return virt_height; }
// bool epd_is_initialized(void) { return initialized; }