#ifndef TIME_O_CLOCK_WIFI_H
#define TIME_O_CLOCK_WIFI_H

#include <esp_err.h>
#include <vector>

typedef struct {
	const char ssid[32];
	const char passwd[64];
} ap_cred_t;

class Wifi{
public:
	enum state_e {
		NOT_INITIALISED,
		INITIALISED,
		DISCONNECTED,
		ACTIVE_SCANNING,
		CONNECTING,
		CONNECTED,
		ERROR
	};
private:
	static state_e state;
	static std::vector<ap_cred_t> known_aps;
public:
	static esp_err_t start();
	static void add_ap(const char *ssid, const char *passwd);
private:
	static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
	static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
};

#endif //TIME_O_CLOCK_WIFI_H
