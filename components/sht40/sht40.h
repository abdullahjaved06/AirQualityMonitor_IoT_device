#ifndef SHT40_H
#define SHT40_H

int sht4x_device_check(void);
float sht4x_read_temperature(void);
float sht4x_read_humidity(void);

#endif /* SHT40_H */
