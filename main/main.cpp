#include <string.h>
#include <ctime>
#include <esp_netif.h>
#include <esp_event.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "webhook.h"
#include "wifi.h"

#include "main.h"

static const char *TAG = "Time O'Clock";

extern "C"{
	void app_main();
}

void app_main(){
	printf("Hello world!\n");

	esp_err_t ret = nvs_flash_init();
	if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	ESP_LOGI(TAG, "Flash Initialised");
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_LOGI(TAG, "TCP/IP Stack enabled.");
	ESP_ERROR_CHECK( esp_event_loop_create_default() );
	initialise_ntp();
//	wifi_init_sta();
	Wifi::add_ap(CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
	Wifi::start();
	wait_for_updated_time();
	Webhook time_oclock_webhook = Webhook(CONFIG_TIME_OCLOCK_WEBHOOK);
	Webhook friday_night_webhook = Webhook(CONFIG_FRIDAY_NIGHT_WEBHOOK);
	char time_str[17];
	int last_time_oclock = -1;
	int last_friday = -1;
	while(1) {
		time_t now;
		struct tm timeinfo;
		time(&now);
		localtime_r(&now, &timeinfo);
		strftime(time_str, 17, "%d/%m/%Y %H:%M",&timeinfo);
		if(timeinfo.tm_year < 120){
			vTaskDelay(5000 / portTICK_PERIOD_MS);
			continue;
		}
		ESP_LOGI("Time O'Clock", "Current Time: %s", time_str);
		if(timeinfo.tm_yday != last_time_oclock && timeinfo.tm_hour == 21){
			time_oclock_webhook.send_message("Hey guys! It's time o'clock!");
			last_time_oclock = timeinfo.tm_yday;
		}
		if(timeinfo.tm_yday != last_friday && timeinfo.tm_wday == 5 && timeinfo.tm_hour == 18){
			friday_night_webhook.send_message("Friday at last... "
											  "https://www.youtube.com/watch?v=DfEnIFV2-mc");
			last_friday = timeinfo.tm_yday;
		}
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}
