#ifndef SENSORS_H
#define SENSORS_H
#include "SGP30.h"

extern sgp30_dev_t *sgp30;
extern sgp30_read_fptr_t sgp30_read;
extern sgp30_write_fptr_t sgp30_write;

void sensor_init();

#endif //SENSORS_H
