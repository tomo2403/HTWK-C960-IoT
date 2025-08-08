#include "sensors.h"

void sensor_init()
{
	sgp30_init(sgp30, sgp30_read, sgp30_write);
	sgp30_IAQ_init(sgp30);
}

void sensor_read()
{
	sgp30_IAQ_measure(sgp30);
}