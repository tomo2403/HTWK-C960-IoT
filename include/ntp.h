#ifndef HTWK_C960_IOT_NTP_H
#define HTWK_C960_IOT_NTP_H

#include "esp_sntp.h"
#include <esp_log.h>

#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"

void time_sync_notification_cb(struct timeval *tv);

void ntp_obtain_time();

#endif //HTWK_C960_IOT_NTP_H
