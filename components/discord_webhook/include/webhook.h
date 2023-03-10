#ifndef TIME_O_CLOCK_WEBHOOK_H
#define TIME_O_CLOCK_WEBHOOK_H

#include <esp_err.h>

class Webhook{
private:
	char *url;
public:
	Webhook(const char *url);
	~Webhook();
	esp_err_t send_message(const char *message);
};

#endif //TIME_O_CLOCK_WEBHOOK_H
