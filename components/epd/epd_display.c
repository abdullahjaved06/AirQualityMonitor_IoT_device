/*
 * EPD Display Driver
 * 
 * Numbers: Arial 28x36 (good!)
 * Labels: Pixel font 8x12 (fixed % and m)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>

#include "epd_display.h"
#include "../../fonts/pixel_font.h"

LOG_MODULE_REGISTER(epd_display, LOG_LEVEL_INF);

/* Arial large number font */
#define LARGE_FONT_WIDTH   28
#define LARGE_FONT_HEIGHT  36
#define LARGE_FONT_FIRST   46
#define LARGE_FONT_LAST    57

static const uint8_t cfb_font_arial_large[] = {
#include "../../fonts/arial_large_data.inc"
};

/* Display state */
static const struct device *display_dev;
static struct display_capabilities caps;
static uint8_t *framebuffer;
static size_t fb_size;
static uint16_t phys_width, phys_height;
static uint16_t virt_width, virt_height;
static uint16_t tile_rows;
static bool initialized = false;

#define MARGIN 5

/* Drawing primitives */
static void set_pixel(uint16_t vx, uint16_t vy, bool black)
{
    if (vx >= virt_width || vy >= virt_height) return;
    uint16_t px = vy;
    uint16_t py = phys_height - 1 - vx;
    if (px >= phys_width || py >= phys_height) return;
    uint16_t tile_row = py / 8;
    uint8_t bit = 7 - (py % 8);
    size_t idx = tile_row * phys_width + px;
    if (idx >= fb_size) return;
    if (black) framebuffer[idx] &= ~(1 << bit);
    else framebuffer[idx] |= (1 << bit);
}

static void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool black)
{
    for (uint16_t py = y; py < y + h && py < virt_height; py++)
        for (uint16_t px = x; px < x + w && px < virt_width; px++)
            set_pixel(px, py, black);
}

static void draw_rect_outline(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool black)
{
    /* Top and bottom */
    for (uint16_t i = 0; i < w; i++) {
        set_pixel(x + i, y, black);
        set_pixel(x + i, y + h - 1, black);
    }
    /* Left and right */
    for (uint16_t i = 0; i < h; i++) {
        set_pixel(x, y + i, black);
        set_pixel(x + w - 1, y + i, black);
    }
}

static void draw_hline(uint16_t x, uint16_t y, uint16_t len, bool black)
{
    for (uint16_t i = 0; i < len; i++) set_pixel(x + i, y, black);
}

static void clear_screen(void) { memset(framebuffer, 0xFF, fb_size); }

/* Large number font (Arial) */
static int draw_vpacked_char(uint16_t x, uint16_t y, const uint8_t *font_data,
                             int font_width, int font_height,
                             int first_char, int last_char, char c)
{
    if (c < first_char || c > last_char) return font_width / 2;
    int char_idx = c - first_char;
    int byte_rows = (font_height + 7) / 8;
    int bytes_per_char = font_width * byte_rows;
    const uint8_t *glyph = &font_data[char_idx * bytes_per_char];
    
    for (int col = 0; col < font_width; col++) {
        for (int byte_row = 0; byte_row < byte_rows; byte_row++) {
            uint8_t byte_val = glyph[byte_row * font_width + col];
            for (int bit = 0; bit < 8; bit++) {
                int py = byte_row * 8 + bit;
                if (py >= font_height) break;
                if (byte_val & (1 << bit)) set_pixel(x + col, y + py, true);
            }
        }
    }
    return font_width;
}

static int draw_large_char(uint16_t x, uint16_t y, char c)
{
    if (c == '.') {
        fill_rect(x + 4, y + LARGE_FONT_HEIGHT - 7, 5, 5, true);
        return 14;
    }
    draw_vpacked_char(x, y, cfb_font_arial_large, LARGE_FONT_WIDTH, LARGE_FONT_HEIGHT,
                      LARGE_FONT_FIRST, LARGE_FONT_LAST, c);
    return 26;
}

static int draw_large_string(uint16_t x, uint16_t y, const char *str)
{
    int start_x = x;
    while (*str) { x += draw_large_char(x, y, *str); str++; }
    return x - start_x;
}

static int get_large_string_width(const char *str)
{
    int width = 0;
    while (*str) { width += (*str == '.') ? 14 : 26; str++; }
    return width;
}

/* Small label font (pixel) */
static int draw_small_char(uint16_t x, uint16_t y, char c)
{
    int idx = pixel_font_index(c);
    const uint8_t *glyph = pixel_font_data[idx];
    
    for (int row = 0; row < PIXEL_FONT_HEIGHT; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < PIXEL_FONT_WIDTH; col++) {
            if (bits & (0x80 >> col)) set_pixel(x + col, y + row, true);
        }
    }
    
    switch(c) {
        case ' ': return 6;
        case '(': case ')': return 6;
        case '%': return 9;
        case 'm': return 9;
        default: return 9;
    }
}

static int draw_small_string(uint16_t x, uint16_t y, const char *str)
{
    int start_x = x;
    while (*str) { x += draw_small_char(x, y, *str); str++; }
    return x - start_x;
}

/* ============================================
 * BATTERY ICON - Based on actual percentage
 * ============================================ */
static void draw_battery_icon(uint16_t x, uint16_t y, int percent)
{
    /* Clamp percentage */
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    
    /* Battery body outline: 20x12 pixels */
    draw_rect_outline(x, y, 20, 12, true);
    
    /* Battery terminal (positive end) */
    fill_rect(x + 20, y + 3, 3, 6, true);
    
    /* Fill level based on percentage */
    /* Inner area is 16x8 (with 2px padding inside) */
    int max_fill = 16;
    int fill_width = (max_fill * percent) / 100;
    
    if (fill_width > 0) {
        fill_rect(x + 2, y + 2, fill_width, 8, true);
    }
    
    LOG_DBG("Battery: %d%%, fill=%d px", percent, fill_width);
}

/* ============================================
 * SIGNAL BARS - Based on RSRP (dBm)
 * ============================================
 * RSRP ranges (typical LTE):
 *   Excellent: > -80 dBm  → 4 bars
 *   Good:      -80 to -90 → 3 bars
 *   Fair:      -90 to -100 → 2 bars
 *   Poor:      -100 to -110 → 1 bar
 *   No signal: < -110      → 0 bars
 * ============================================ */
static int rsrp_to_bars(int rsrp_dbm)
{
    if (rsrp_dbm == 0) return 0;        /* No reading */
    if (rsrp_dbm > -80) return 4;       /* Excellent */
    if (rsrp_dbm > -90) return 3;       /* Good */
    if (rsrp_dbm > -100) return 2;      /* Fair */
    if (rsrp_dbm > -110) return 1;      /* Poor */
    return 0;                            /* No signal */
}

static void draw_signal_bars(uint16_t x, uint16_t y, int rsrp_dbm)
{
    int num_bars = rsrp_to_bars(rsrp_dbm);
    
    /* Bar dimensions */
    int bar_width = 3;
    int bar_spacing = 2;
    int bar_heights[] = {3, 5, 7, 10};  /* Heights for bars 1-4 */
    int max_height = 10;
    
    for (int i = 0; i < 4; i++) {
        int bar_x = x + i * (bar_width + bar_spacing);
        int bar_h = bar_heights[i];
        int bar_y = y + (max_height - bar_h);  /* Align to bottom */
        
        if (i < num_bars) {
            /* Filled bar (active) */
            fill_rect(bar_x, bar_y, bar_width, bar_h, true);
        } else {
            /* Empty bar (outline only) */
            draw_rect_outline(bar_x, bar_y, bar_width, bar_h, true);
        }
    }
    
    LOG_DBG("Signal: %d dBm → %d bars", rsrp_dbm, num_bars);
}

/* ============================================
 * CHARGING ICON 
 * ============================================ */
static void draw_charging_icon(uint16_t x, uint16_t y)
{
    /* Simple plug icon */
    /*
     *   █ █
     *   █ █
     *   ███
     *   ███
     *    █
     */
    
    /* Two prongs at top */
    fill_rect(x + 1, y + 0, 2, 3, true);   /* Left prong */
    fill_rect(x + 5, y + 0, 2, 3, true);   /* Right prong */
    
    /* Plug body */
    fill_rect(x + 0, y + 3, 8, 4, true);
    
    /* Cable */
    fill_rect(x + 3, y + 7, 2, 3, true);
}

/* Display API */
static void refresh_display(void)
{
    struct display_buffer_descriptor desc = {
        .buf_size = fb_size, .width = phys_width, .height = phys_height, .pitch = phys_width,
    };
    display_write(display_dev, 0, 0, &desc, framebuffer);
}

#define FB_STATIC_SIZE 4096
static uint8_t framebuffer_static[FB_STATIC_SIZE];

int epd_init(void)
{
    if (initialized) return 0;
    
    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display_dev)) {
        LOG_ERR("EPD: Device not ready");
        return -ENODEV;
    }
    
    display_get_capabilities(display_dev, &caps);
    phys_width = caps.x_resolution;
    phys_height = caps.y_resolution;
    tile_rows = (phys_height + 7) / 8;
    fb_size = phys_width * tile_rows;
    virt_width = phys_height;
    virt_height = phys_width;
    
    LOG_INF("EPD: %dx%d, fb_size=%d", phys_width, phys_height, fb_size);
    
    if (fb_size > FB_STATIC_SIZE) {
        LOG_ERR("EPD: Static buffer too small! Need %d, have %d", fb_size, FB_STATIC_SIZE);
        return -ENOMEM;
    }
    
    framebuffer = framebuffer_static;
    clear_screen();
    display_blanking_off(display_dev);
    
    initialized = true;
    LOG_INF("EPD: Init complete, virt=%dx%d", virt_width, virt_height);
    
    return 0;
}

void epd_clear(void) { if (initialized) clear_screen(); }
void epd_refresh(void) { if (initialized) refresh_display(); }

/* ============================================
 * MAIN UI DRAWING FUNCTION
 * ============================================ */
void epd_draw_ui(int co2_ppm, float temperature, float humidity,
                 int battery_percent, bool charging, int signal_rsrp)
{
    if (!initialized) return;
    
    LOG_INF("EPD: draw_ui - CO2=%d, T=%.1f, H=%.1f, Batt=%d%%, RSRP=%d",
            co2_ppm, (double)temperature, (double)humidity, 
            battery_percent, signal_rsrp);
    
    char buf[16];
    int content_width = virt_width - (2 * MARGIN);
    clear_screen();
    
    int header_h = 20;
    int section_h = (virt_height - header_h) / 3;
    int label_y = 5;
    int number_y = 28;
    
    /* ========== HEADER ========== */
    int icon_y = 4;
    
    /* Battery icon (left side) */
    draw_battery_icon(MARGIN, icon_y, battery_percent);
    
    /* Signal bars (after battery, with gap) */
    int sig_x = MARGIN + 28;
    draw_signal_bars(sig_x, icon_y, signal_rsrp);
    
    /* Charging icon (right side) */
    if (charging) {
        int chg_x = virt_width - MARGIN - 14;
        draw_charging_icon(chg_x, icon_y);
    }
    
    /* Header separator line */
    draw_hline(MARGIN, header_h, content_width, true);
    
    /* ========== CO2 SECTION ========== */
    int sec_y = header_h;
    draw_small_string(MARGIN, sec_y + label_y, "CO2 (ppm)");
    snprintf(buf, sizeof(buf), "%d", co2_ppm);
    int w = get_large_string_width(buf);
    draw_large_string((virt_width - w) / 2, sec_y + number_y, buf);
    draw_hline(MARGIN, header_h + section_h, content_width, true);
    
    /* ========== HUMIDITY SECTION ========== */
    sec_y = header_h + section_h;
    draw_small_string(MARGIN, sec_y + label_y, "HUM (%)");
    snprintf(buf, sizeof(buf), "%d", (int)humidity);
    w = get_large_string_width(buf);
    draw_large_string((virt_width - w) / 2, sec_y + number_y, buf);
    draw_hline(MARGIN, header_h + section_h * 2, content_width, true);
    
    /* ========== TEMPERATURE SECTION ========== */
    sec_y = header_h + section_h * 2;
    draw_small_string(MARGIN, sec_y + label_y, "TEMP (C)");
    int t_int = (int)temperature;
    int t_dec = (int)((temperature - t_int) * 10);
    if (t_dec < 0) t_dec = -t_dec;
    snprintf(buf, sizeof(buf), "%d.%d", t_int, t_dec);
    w = get_large_string_width(buf);
    draw_large_string((virt_width - w) / 2, sec_y + number_y, buf);
    
    /* Refresh display */
    LOG_INF("EPD: Refreshing display...");
    refresh_display();
    LOG_INF("EPD: Done");
}

uint16_t epd_get_width(void) { return virt_width; }
uint16_t epd_get_height(void) { return virt_height; }
bool epd_is_initialized(void) { return initialized; }