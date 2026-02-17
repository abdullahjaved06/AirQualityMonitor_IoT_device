/*
 * Display Test - Script-Generated Arial Fonts
 * Portrait mode with manual rotation
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "display.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    int ret;
    
    LOG_INF("=== EPD Display Test (Arial Fonts) ===");
    
    /* Initialize display */
    ret = epd_init();
    if (ret != 0) {
        LOG_ERR("Display init failed: %d", ret);
        return -1;
    }
    
    LOG_INF("Display: %dx%d (portrait)", epd_get_width(), epd_get_height());
    
    /* Draw initial UI with test values */
    LOG_INF("Drawing UI with Arial fonts...");
    epd_draw_ui(1234, 23.5f, 45.0f, 75, false);
    
    LOG_INF("Success! Display should show Aranet-style UI");
    
    /* Continuous updates for testing */
    int co2 = 400;
    int humidity = 40;
    float temp = 22.0f;
    bool charging = false;
    int battery = 85;
    
    while (1) {
        k_msleep(30000);  /* 30 second updates */
        
        /* Simulate changing values */
        co2 += 50;
        if (co2 > 2500) co2 = 400;
        
        humidity += 3;
        if (humidity > 85) humidity = 35;
        
        temp += 0.3f;
        if (temp > 32.0f) temp = 18.0f;
        
        battery -= 2;
        if (battery < 5) {
            battery = 100;
            charging = !charging;
        }
        
        LOG_INF("Update: CO2=%d ppm, Temp=%.1f C, Hum=%d%%, Batt=%d%% %s",
                co2, (double)temp, humidity, battery,
                charging ? "(charging)" : "");
        
        epd_draw_ui(co2, temp, (float)humidity, battery, charging);
    }
    
    return 0;
}