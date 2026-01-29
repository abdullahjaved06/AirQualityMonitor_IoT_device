/*
 * Aesthetic EPD Display Driver - Portrait Mode
 * Clean design - white background, balanced spacing
 * Fixed signal bars
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>

LOG_MODULE_REGISTER(epd_display, LOG_LEVEL_INF);

/*
 * 8x16 font
 */
static const uint8_t font_8x16[][16] = {
    /* '0' */ {0x3C,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x3C,0x00},
    /* '1' */ {0x08,0x18,0x28,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x3E,0x00},
    /* '2' */ {0x3C,0x42,0x42,0x02,0x02,0x04,0x08,0x10,0x20,0x40,0x40,0x40,0x40,0x42,0x7E,0x00},
    /* '3' */ {0x3C,0x42,0x42,0x02,0x02,0x02,0x1C,0x02,0x02,0x02,0x02,0x02,0x42,0x42,0x3C,0x00},
    /* '4' */ {0x04,0x0C,0x14,0x24,0x44,0x44,0x84,0x84,0xFE,0x04,0x04,0x04,0x04,0x04,0x04,0x00},
    /* '5' */ {0x7E,0x40,0x40,0x40,0x40,0x78,0x44,0x02,0x02,0x02,0x02,0x02,0x42,0x44,0x38,0x00},
    /* '6' */ {0x1C,0x20,0x40,0x40,0x40,0x78,0x44,0x42,0x42,0x42,0x42,0x42,0x42,0x24,0x18,0x00},
    /* '7' */ {0x7E,0x42,0x02,0x04,0x04,0x08,0x08,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00},
    /* '8' */ {0x3C,0x42,0x42,0x42,0x42,0x42,0x3C,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x3C,0x00},
    /* '9' */ {0x18,0x24,0x42,0x42,0x42,0x42,0x42,0x26,0x1A,0x02,0x02,0x02,0x02,0x04,0x38,0x00},
    /* ' ' */ {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* ':' */ {0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00},
    /* '%' */ {0x00,0x62,0x92,0x94,0x64,0x08,0x08,0x10,0x10,0x26,0x29,0x49,0x46,0x00,0x00,0x00},
    /* '.' */ {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00},
    /* 'C' */ {0x1C,0x22,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x22,0x22,0x1C,0x00},
    /* 'H' */ {0x42,0x42,0x42,0x42,0x42,0x42,0x7E,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x00},
    /* 'T' */ {0xFE,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00},
    /* 'p' */ {0x00,0x00,0x00,0x00,0x00,0x5C,0x62,0x42,0x42,0x42,0x62,0x5C,0x40,0x40,0x40,0x00},
    /* 'm' */ {0x00,0x00,0x00,0x00,0x00,0x76,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x00},
    /* 'u' */ {0x00,0x00,0x00,0x00,0x00,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x46,0x3A,0x00},
    /* 'i' */ {0x00,0x10,0x10,0x00,0x00,0x30,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x38,0x00},
    /* 'O' */ {0x3C,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x3C,0x00},
    /* 'e' */ {0x00,0x00,0x00,0x00,0x00,0x3C,0x42,0x42,0x42,0x7E,0x40,0x40,0x40,0x42,0x3C,0x00},
    /* '-' */ {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* 'a' */ {0x00,0x00,0x00,0x00,0x00,0x3C,0x42,0x02,0x3E,0x42,0x42,0x42,0x42,0x46,0x3A,0x00},
    /* 'r' */ {0x00,0x00,0x00,0x00,0x00,0x5C,0x62,0x42,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x00},
    /* 'g' */ {0x00,0x00,0x00,0x00,0x00,0x3A,0x46,0x42,0x42,0x42,0x42,0x46,0x3A,0x02,0x3C,0x00},
    /* 'n' */ {0x00,0x00,0x00,0x00,0x00,0x5C,0x62,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x00},
    /* 'B' */ {0x7C,0x42,0x42,0x42,0x42,0x7C,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x7C,0x00},
    /* 't' */ {0x00,0x10,0x10,0x10,0x10,0x7C,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x12,0x0C,0x00},
    /* 'E' */ {0x7E,0x40,0x40,0x40,0x40,0x40,0x7C,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x7E,0x00},
    /* 'M' */ {0x82,0xC6,0xAA,0x92,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x00},
    /* 'P' */ {0x7C,0x42,0x42,0x42,0x42,0x42,0x7C,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x00},
    /* 'R' */ {0x7C,0x42,0x42,0x42,0x42,0x42,0x7C,0x48,0x44,0x44,0x42,0x42,0x42,0x42,0x42,0x00},
    /* 'A' */ {0x18,0x24,0x42,0x42,0x42,0x42,0x42,0x7E,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x00},
    /* 'U' */ {0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x24,0x18,0x00},
    /* 'I' */ {0x3E,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x3E,0x00},
    /* 'D' */ {0x78,0x44,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x44,0x78,0x00},
    /* 'Y' */ {0x82,0x82,0x44,0x44,0x28,0x28,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00},
    /* 'F' */ {0x7E,0x40,0x40,0x40,0x40,0x40,0x7C,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x00},
    /* 'o' */ {0x00,0x00,0x00,0x00,0x00,0x3C,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x3C,0x00},
};

static int get_font_index(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    switch (c) {
        case ' ': return 10;
        case ':': return 11;
        case '%': return 12;
        case '.': return 13;
        case 'C': return 14;
        case 'H': return 15;
        case 'T': return 16;
        case 'p': return 17;
        case 'm': return 18;
        case 'u': return 19;
        case 'i': return 20;
        case 'O': return 21;
        case 'e': return 22;
        case '-': return 23;
        case 'a': return 24;
        case 'r': return 25;
        case 'g': return 26;
        case 'n': return 27;
        case 'B': return 28;
        case 't': return 29;
        case 'E': return 30;
        case 'M': return 31;
        case 'P': return 32;
        case 'R': return 33;
        case 'A': return 34;
        case 'U': return 35;
        case 'I': return 36;
        case 'D': return 37;
        case 'Y': return 38;
        case 'F': return 39;
        case 'o': return 40;
        default: return 10;
    }
}

/* Display state */
static const struct device *display_dev;
static struct display_capabilities caps;
static uint8_t *framebuffer;
static size_t fb_size;
static uint16_t phys_width, phys_height;
static uint16_t virt_width, virt_height;
static uint16_t tile_rows;
static bool initialized = false;

/* Margins */
#define MARGIN 10

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
    
    if (black) {
        framebuffer[idx] &= ~(1 << bit);
    } else {
        framebuffer[idx] |= (1 << bit);
    }
}

static void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool black)
{
    for (uint16_t py = y; py < y + h && py < virt_height; py++) {
        for (uint16_t px = x; px < x + w && px < virt_width; px++) {
            set_pixel(px, py, black);
        }
    }
}

static void draw_hline(uint16_t x, uint16_t y, uint16_t len, bool black)
{
    for (uint16_t i = 0; i < len; i++) {
        set_pixel(x + i, y, black);
    }
}

static void draw_char_scaled(uint16_t x, uint16_t y, char c, bool black, int scale)
{
    int idx = get_font_index(c);
    if (idx < 0 || idx >= 41) idx = 10;
    const uint8_t *glyph = font_8x16[idx];
    
    for (int row = 0; row < 16; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        set_pixel(x + col * scale + sx, y + row * scale + sy, black);
                    }
                }
            }
        }
    }
}

static int draw_string_scaled(uint16_t x, uint16_t y, const char *str, bool black, int scale)
{
    int start_x = x;
    while (*str) {
        draw_char_scaled(x, y, *str, black, scale);
        x += 8 * scale;
        str++;
    }
    return x - start_x;
}

static void clear_screen(void)
{
    memset(framebuffer, 0xFF, fb_size);
}

static void refresh_display(void)
{
    struct display_buffer_descriptor desc = {
        .buf_size = fb_size,
        .width = phys_width,
        .height = phys_height,
        .pitch = phys_width,
    };
    display_write(display_dev, 0, 0, &desc, framebuffer);
}

/*
 * Public API
 */

int epd_init(void)
{
    if (initialized) return 0;
    
    LOG_INF("Initializing EPD display...");
    
    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device not ready");
        return -ENODEV;
    }
    
    display_get_capabilities(display_dev, &caps);
    phys_width = caps.x_resolution;
    phys_height = caps.y_resolution;
    tile_rows = (phys_height + 7) / 8;
    fb_size = phys_width * tile_rows;
    
    virt_width = phys_height;
    virt_height = phys_width;
    
    LOG_INF("Display: %dx%d (portrait)", virt_width, virt_height);
    
    framebuffer = k_malloc(fb_size);
    if (!framebuffer) {
        LOG_ERR("Failed to allocate framebuffer");
        return -ENOMEM;
    }
    
    clear_screen();
    display_blanking_off(display_dev);
    
    initialized = true;
    return 0;
}

void epd_clear(void)
{
    if (!initialized) return;
    clear_screen();
}

void epd_refresh(void)
{
    if (!initialized) return;
    refresh_display();
}

void epd_draw_ui(int co2_ppm, float temperature, float humidity,
                 int battery_percent, bool charging)
{
    if (!initialized) return;
    
    char buf[16];
    int content_width = virt_width - (2 * MARGIN);
    
    clear_screen();
    
    int header_h = 22;
    int section_h = (virt_height - header_h) / 3;
    
    /* ============================================ */
    /* HEADER BAR                                  */
    /* ============================================ */
    
    /* Battery icon (left) - position */
    int batt_x = MARGIN;
    int batt_y = 5;
    
    /* Battery outline */
    for (int i = 0; i <= 16; i++) {
        set_pixel(batt_x + i, batt_y, true);
        set_pixel(batt_x + i, batt_y + 10, true);
    }
    for (int i = 0; i <= 10; i++) {
        set_pixel(batt_x, batt_y + i, true);
        set_pixel(batt_x + 16, batt_y + i, true);
    }
    /* Battery tip */
    for (int i = 3; i <= 7; i++) {
        set_pixel(batt_x + 17, batt_y + i, true);
        set_pixel(batt_x + 18, batt_y + i, true);
    }
    /* Battery fill */
    int fill = (12 * battery_percent) / 100;
    if (fill > 0) {
        fill_rect(batt_x + 2, batt_y + 2, fill, 7, true);
    }
    
    /* Signal bars - right after battery with small gap */
    int sig_x = batt_x + 22;  /* Right after battery */
    int sig_y = 5;
    int signal_strength = 4;  /* 0-4 (4 bars total) */
    
    /* 4 bars with increasing height, all aligned at bottom */
    int bar_w = 3;
    int bar_gap = 2;
    int bar_bottom = sig_y + 12;  /* All bars align to this bottom */
    int bar_heights[] = {4, 7, 10, 13};  /* Heights for 4 bars */
    
    for (int i = 0; i < 4; i++) {
        int bx = sig_x + i * (bar_w + bar_gap);
        int h = bar_heights[i];
        int by = bar_bottom - h;
        
        if (i < signal_strength) {
            /* Filled bar */
            fill_rect(bx, by, bar_w, h, true);
        } else {
            /* Empty bar - just outline */
            for (int px = bx; px < bx + bar_w; px++) {
                set_pixel(px, bar_bottom - 1, true);  /* Bottom line only */
            }
        }
    }
    
    /* Charger icon (right side) */
    if (charging) {
        int chg_x = virt_width - MARGIN - 14;
        int chg_y = 4;
        /* Plug prongs */
        fill_rect(chg_x + 2, chg_y, 2, 4, true);
        fill_rect(chg_x + 8, chg_y, 2, 4, true);
        /* Plug body */
        fill_rect(chg_x, chg_y + 4, 12, 4, true);
        /* Cable */
        fill_rect(chg_x + 4, chg_y + 8, 4, 6, true);
    }
    
    /* Header separator line */
    draw_hline(MARGIN, header_h, content_width, true);
    
    /* ============================================ */
    /* CO2 SECTION                                 */
    /* ============================================ */
    int y = header_h + 8;
    
    /* "CO2" label */
    draw_string_scaled(MARGIN, y, "CO", true, 1);
    draw_char_scaled(MARGIN + 16, y + 6, '2', true, 1);
    
    /* CO2 value */
    y += 22;
    snprintf(buf, sizeof(buf), "%d", co2_ppm);
    int num_w = draw_string_scaled(MARGIN, y, buf, true, 2);
    
    /* "ppm" unit */
    draw_string_scaled(MARGIN + num_w + 4, y + 16, "ppm", true, 1);
    
    /* Section separator */
    y = header_h + section_h;
    draw_hline(MARGIN, y, content_width, true);
    
    /* ============================================ */
    /* HUMIDITY SECTION                            */
    /* ============================================ */
    y += 8;
    
    draw_string_scaled(MARGIN, y, "HUMIDITY", true, 1);
    
    y += 22;
    int humi_int = (int)humidity;
    snprintf(buf, sizeof(buf), "%d", humi_int);
    num_w = draw_string_scaled(MARGIN, y, buf, true, 2);
    
    draw_string_scaled(MARGIN + num_w + 4, y + 16, "%", true, 1);
    
    /* Section separator */
    y = header_h + (section_h * 2);
    draw_hline(MARGIN, y, content_width, true);
    
    /* ============================================ */
    /* TEMPERATURE SECTION                         */
    /* ============================================ */
    y += 8;
    
    draw_string_scaled(MARGIN, y, "TEMPERATURE", true, 1);
    
    y += 22;
    int temp_int = (int)temperature;
    int temp_dec = (int)((temperature - temp_int) * 10);
    if (temp_dec < 0) temp_dec = -temp_dec;
    snprintf(buf, sizeof(buf), "%d.%d", temp_int, temp_dec);
    num_w = draw_string_scaled(MARGIN, y, buf, true, 2);
    
    draw_string_scaled(MARGIN + num_w + 4, y + 16, "C", true, 1);
}

uint16_t epd_get_width(void) { return virt_width; }
uint16_t epd_get_height(void) { return virt_height; }