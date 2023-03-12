#include <esp_wifi.h>
#include <esp_log.h>
#include <cstring>
#include "wifi.h"

#define DEFAULT_SCAN_LIST_SIZE 5

const static char *TAG = "WiFi";


Wifi::state_e Wifi::state = Wifi::NOT_INITIALISED;
std::vector<ap_cred_t> Wifi::known_aps = std::vector<ap_cred_t>();


esp_err_t Wifi::start() {
	// Already initialised. Skip doing it again...
	if(state != NOT_INITIALISED) return ESP_OK;
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

	esp_netif_create_default_wifi_sta();

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

	if(event_id == WIFI_EVENT_STA_DISCONNECTED || event_id == WIFI_EVENT_STA_START){
		Wifi::state = Wifi::DISCONNECTED;
		esp_wifi_scan_start(nullptr, false);
		ESP_LOGI(TAG, "Scanning Started...");
		Wifi::state = Wifi::ACTIVE_SCANNING;
	}else if(event_id == WIFI_EVENT_STA_CONNECTED){
		Wifi::state = Wifi::CONNECTED;
	}else if(event_id == WIFI_EVENT_SCAN_DONE){
		esp_wifi_scan_get_ap_records(&number, ap_info);
		int8_t best_rssi = -127;
		ap_cred_t best_network = {};
		for(int i=0; i<number; ++i){
			if(ap_info[i].authmode != WIFI_AUTH_WPA2_WPA3_PSK && ap_info[i].authmode != WIFI_AUTH_WPA2_PSK && ap_info[i].authmode != WIFI_AUTH_WPA3_PSK){
				continue;
			}
			for(ap_cred_t cred : Wifi::known_aps){
				if(strcmp(reinterpret_cast<const char *>(ap_info[i].ssid), cred.ssid) != 0){
					// Scanned AP is not in the known APs list
					continue;
				}
				ESP_LOGI(TAG, "Best RSSI: %d", best_rssi);
				ESP_LOGI(TAG, "SSID: %s, RSSI: %d", ap_info[i].ssid, ap_info[i].rssi);
				if(ap_info[i].rssi > best_rssi){
					ESP_LOGI(TAG, "Found better network");
					best_rssi = ap_info[i].rssi;
					strcpy(const_cast<char *>(best_network.ssid), cred.ssid);
					strcpy(const_cast<char *>(best_network.passwd), cred.passwd);
				}
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
			memcpy(reinterpret_cast<char *>(config.sta.ssid), best_network.ssid, strlen(best_network.ssid)+1);
			memcpy(reinterpret_cast<char *>(config.sta.password), best_network.passwd, strlen(best_network.passwd)+1);
			ESP_LOGI(TAG, "%p %p", best_network.ssid, best_network.passwd);
			ESP_LOGI(TAG, "Connecting to: %s", best_network.ssid);
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

void Wifi::add_ap(const char *ssid, const char *passwd) {
	ap_cred_t ap_cred = {};
	strcpy(const_cast<char *>(ap_cred.ssid), ssid);
	strcpy(const_cast<char *>(ap_cred.passwd), passwd);
//	ESP_LOGI(TAG, "Adding AP: %s, %s", ssid, passwd);
//	ESP_LOGI(TAG, "Copied AP to credentials: %s, %s", ap_cred.ssid, ap_cred.passwd);
	Wifi::known_aps.push_back(ap_cred);
}

void Wifi::ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {

}
