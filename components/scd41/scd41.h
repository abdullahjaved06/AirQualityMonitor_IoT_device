#ifndef SCD41_H
#define SCD41_H

int scd41_device_check(void);
float scd41_read_co2(void);

//using them as extern to avoid multiple definition error.
extern float co2_medium_threshold;
extern float co2_high_threshold;



#endif /* SCD41_H */
