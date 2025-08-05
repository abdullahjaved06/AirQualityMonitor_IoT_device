#ifndef SHT40_H
#define SHT40_H

int sht4x_device_check(void);
float sht4x_read_temperature(void);
float sht4x_read_humidity(void);

//using them as extern to avoid multiple definition error.
extern float temp_high_threshold;
extern float temp_low_threshold;

extern float hum_high_threshold;
extern float hum_low_threshold;


#endif /* SHT40_H */
