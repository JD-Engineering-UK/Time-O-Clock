#include "main.h"

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <esp_netif.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
//#include "protocol_examples_common.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"

static const char *TAG = "example";

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;

static void obtain_time(void);

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_CUSTOM
void sntp_sync_time(struct timeval *tv)
{
   settimeofday(tv, NULL);
   ESP_LOGI(TAG, "Time is synchronized from custom code");
   sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}
#endif

void time_sync_notification_cb(struct timeval *tv)
{
	ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void print_servers(void)
{
	ESP_LOGI(TAG, "List of configured NTP servers:");

	for (uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i){
		if (esp_sntp_getservername(i)){
			ESP_LOGI(TAG, "server %d: %s", i, esp_sntp_getservername(i));
		} else {
			// we have either IPv4 or IPv6 address, let's print it
			char buff[INET6_ADDRSTRLEN];
			ip_addr_t const *ip = esp_sntp_getserver(i);
			if (ipaddr_ntoa_r(ip, buff, INET6_ADDRSTRLEN) != NULL)
				ESP_LOGI(TAG, "server %d: %s", i, buff);
		}
	}
}

void initialise_ntp(){
	ESP_LOGI(TAG, "Initializing SNTP");
	setenv("TZ", "BST0GMT,M3.2.0/2:00:00,M11.1.0/2:00:00",1);
	tzset();
	esp_sntp_config_t config = {
		.smooth_sync=true,
		.server_from_dhcp=false,
		.wait_for_sync=true,
		.start=true,
		.sync_cb=time_sync_notification_cb,
		.renew_servers_after_new_IP=false,
		.ip_event_to_renew=IP_EVENT_STA_GOT_IP,
		.index_of_first_server=0,
		.num_of_servers=(1),
		.servers={
		CONFIG_SNTP_TIME_SERVER},
		};
	esp_netif_sntp_init(&config);
}

void wait_for_updated_time(){
	int retry_count=10;
	int retry=0;
	while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
		ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
	}
}
