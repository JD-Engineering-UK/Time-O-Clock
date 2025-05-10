#include <esp_wifi.h>
#include <esp_log.h>
#include <cstring>
#include "wifi.h"
#include "files.h"
#include "credentials.h"

#define DEFAULT_SCAN_LIST_SIZE 5

const static char *TAG = "WiFi";

Wifi::state_e Wifi::state = Wifi::NOT_INITIALISED;

esp_err_t Wifi::start() {
	// Already initialised. Skip doing it again...
	if(state != NOT_INITIALISED) return ESP_OK;
	init_credentials();

	esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
	wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();

	esp_err_t err = esp_wifi_init(&config);
	if(err != ESP_OK){
		ESP_LOGE(TAG, "WiFi init error: %s", esp_err_to_name(err));
		return err;
	}

	esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, nullptr);
	esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, nullptr, nullptr);

	err = esp_wifi_set_mode(WIFI_MODE_STA);
	if(err != ESP_OK){
		ESP_LOGE(TAG, "WiFi set mode error: %s", esp_err_to_name(err));
		return err;
	}

	err = esp_wifi_start();
	if(err != ESP_OK){
		ESP_LOGE(TAG, "WiFi start error: %s", esp_err_to_name(err));
		return err;
	}

	state = INITIALISED;
	ESP_LOGI(TAG, "WiFi Initialised.");

	return ESP_OK;
}

void Wifi::wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	// Skip processing non-WiFi events
	if(event_base != WIFI_EVENT) return;
//	ESP_LOGI(TAG, "WiFi Event");
	wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
	uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    uint16_t ap_count = 0;

	if(event_id == WIFI_EVENT_STA_DISCONNECTED || event_id == WIFI_EVENT_STA_START){
		Wifi::state = Wifi::DISCONNECTED;
		esp_wifi_scan_start(nullptr, false);
		ESP_LOGI(TAG, "Scanning Started...");
		Wifi::state = Wifi::ACTIVE_SCANNING;
	}else if(event_id == WIFI_EVENT_STA_CONNECTED){
		Wifi::state = Wifi::CONNECTED;
	}else if(event_id == WIFI_EVENT_SCAN_DONE){
		esp_wifi_scan_get_ap_num(&ap_count);
		esp_wifi_scan_get_ap_records(&number, ap_info);
		int8_t best_rssi = -127;
		cred_t *best_network;

		ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);
		for(int i=0; i<number; ++i){
			ESP_LOGI(TAG, "Seen NW: %s", ap_info[i].ssid);
			if(ap_info[i].authmode != WIFI_AUTH_WPA2_WPA3_PSK && ap_info[i].authmode != WIFI_AUTH_WPA2_PSK && ap_info[i].authmode != WIFI_AUTH_WPA3_PSK){
				continue;
			}
			cred_t * known_ap = get_credentials(ap_info[i].ssid, sizeof ap_info[i].ssid);
			if(!known_ap)
			{	
				ESP_LOGD(TAG, "Network not known");
				continue;
			}
			ESP_LOGI(TAG, "Best RSSI: %d", best_rssi);
			ESP_LOGI(TAG, "SSID: %s, RSSI: %d", ap_info[i].ssid, ap_info[i].rssi);
			if(ap_info[i].rssi > best_rssi){
				ESP_LOGI(TAG, "Found better network");
				best_rssi = ap_info[i].rssi;
				best_network = known_ap;
			}
		}
		ESP_LOGI(TAG, "Best RSSI found: %d", best_rssi);
		if(best_rssi > -127){
			// Found an AP to connect to...
			wifi_config_t config = {
				.sta = {
					.ssid = {0},
					.password = {0},
					.scan_method = WIFI_FAST_SCAN,
					.sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
//					.threshold = {
//						.rssi =
//						.authmode = WIFI_AUTH_WPA2_PSK
//					}
				}
			};
			memcpy(reinterpret_cast<char *>(config.sta.ssid), best_network->ssid, best_network->ssid_len);
			memcpy(reinterpret_cast<char *>(config.sta.password), best_network->password, best_network->passwd_len);
			char local_buf[33] = {0};
			memcpy(local_buf, best_network->ssid, best_network->ssid_len);
			ESP_LOGI(TAG, "Connecting to: %s", local_buf);
			esp_wifi_set_config(WIFI_IF_STA, &config);
			ESP_ERROR_CHECK(esp_wifi_connect());
			Wifi::state = Wifi::CONNECTING;
		}else{
			esp_wifi_scan_start(nullptr, false);
			ESP_LOGI(TAG, "Scanning for APs again...");
			Wifi::state = Wifi::ACTIVE_SCANNING;
		}
	}else if(event_id == WIFI_EVENT_STA_BSS_RSSI_LOW){
		esp_wifi_disconnect();
	}
}

void Wifi::ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {

}
