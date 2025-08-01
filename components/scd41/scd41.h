#ifndef SCD41_H
#define SCD41_H

int scd41_device_check(void);
float scd41_read_co2(void);


// --- Thresholds ---
#define CO2_MEDIUM_THRESHOLD    1000.0f // ppm (low-priority alert)
#define CO2_HIGH_THRESHOLD      2000.0f // ppm (high-priority alert)


#endif /* SCD41_H */
