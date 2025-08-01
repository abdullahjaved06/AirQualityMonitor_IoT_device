#ifndef SHT40_H
#define SHT40_H

int sht4x_device_check(void);
float sht4x_read_temperature(void);
float sht4x_read_humidity(void);

// --- Thresholds ---

#define TEMP_HIGH_THRESHOLD     30.0f   // °C
#define TEMP_LOW_THRESHOLD      5.0f

#define HUM_HIGH_THRESHOLD      80.0f   // %
#define HUM_LOW_THRESHOLD       20.0f

#endif /* SHT40_H */
