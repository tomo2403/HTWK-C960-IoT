#include "sensors.h"

sgp30_dev_t *sgp30;
sgp30_read_fptr_t sgp30_read;
sgp30_write_fptr_t sgp30_write;

void sensor_init()
{
	sgp30_init(sgp30, sgp30_read, sgp30_write);
	sgp30_IAQ_init(sgp30);
}

void sensor_read()
{
	sgp30_IAQ_measure(sgp30);
}