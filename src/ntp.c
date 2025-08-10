#include "../include/ntp.h"

static const char *TAG = "TimeManager";

void time_sync_notification_cb(struct timeval *tv)
{
	ESP_LOGI(TAG, "Zeit synchronisiert");
}

void ntp_obtain_time()
{
	esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
	esp_sntp_setservername(0, "pool.ntp.org");
	sntp_set_time_sync_notification_cb(time_sync_notification_cb);
	esp_sntp_init();

	setenv("TZ", TIMEZONE, 1);
	tzset();

	time_t now;
	struct tm timeInfo;
	int retry = 0;
	const int retry_count = 10;

	do
	{
		time(&now);
		localtime_r(&now, &timeInfo);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	} while (timeInfo.tm_year < (2016 - 1900) && ++retry < retry_count);

	if (retry < retry_count)
	{
		ESP_LOGI(TAG, "Zeit: %s", asctime(&timeInfo));
	}
	else
	{
		ESP_LOGW(TAG, "NTP Synchronisierung fehlgeschlagen");
	}
}
