#include <cstring>
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_tls.h>
#include <sys/param.h>
#include <esp_crt_bundle.h>
#include "webhook.h"

#define MAX_HTTP_OUTPUT_BUFFER 256
const char *TAG = "WH";


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
	static char *output_buffer;  // Buffer to store response of http request from event handler
	static int output_len = 0;       // Stores number of bytes read
	int mbedtls_err;
	esp_err_t err;
	switch(evt->event_id) {
		case HTTP_EVENT_ERROR:
			ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
			break;
		case HTTP_EVENT_ON_CONNECTED:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
			break;
		case HTTP_EVENT_HEADER_SENT:
			ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
			break;
		case HTTP_EVENT_ON_HEADER:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
			break;
		case HTTP_EVENT_ON_DATA:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
			/*
			 *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
			 *  However, event handler can also be used in case chunked encoding is used.
			 */
			if (!esp_http_client_is_chunked_response(evt->client)) {
				// If user_data buffer is configured, copy the response into the buffer
				int copy_len = 0;
				if (evt->user_data) {
					copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
					if (copy_len) {
						memcpy(evt->user_data + output_len, evt->data, copy_len);
					}
				} else {
					const int buffer_len = esp_http_client_get_content_length(evt->client);
					if (output_buffer == NULL) {
						output_buffer = (char *) malloc(buffer_len);
						output_len = 0;
						if (output_buffer == NULL) {
							ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
							return ESP_FAIL;
						}
					}
					copy_len = MIN(evt->data_len, (buffer_len - output_len));
					if (copy_len) {
						memcpy(output_buffer + output_len, evt->data, copy_len);
					}
				}
				output_len += copy_len;
			}

			break;
		case HTTP_EVENT_ON_FINISH:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
			if (output_buffer != NULL) {
				// Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
				// ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
				free(output_buffer);
				output_buffer = NULL;
			}
			output_len = 0;
			break;
		case HTTP_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
			mbedtls_err = 0;
			err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
			if (err != 0) {
				ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
				ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
			}
			if (output_buffer != NULL) {
				free(output_buffer);
				output_buffer = NULL;
			}
			output_len = 0;
			break;
		case HTTP_EVENT_REDIRECT:
			ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
			esp_http_client_set_header(evt->client, "From", "user@example.com");
			esp_http_client_set_header(evt->client, "Accept", "text/html");
			esp_http_client_set_redirection(evt->client);
			break;
	}
	return ESP_OK;
}



Webhook::Webhook(const char *url) {
	size_t url_len = strlen(url);
	this->url = static_cast<char *>(malloc(url_len+1));
	memcpy(this->url, url, url_len +1);
}

Webhook::~Webhook() {
	free(this->url);
}


esp_err_t Webhook::send_message(const char *message) {
	esp_http_client_config_t config = {
		.url = url,
		.event_handler = _http_event_handler,
		.crt_bundle_attach = esp_crt_bundle_attach
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// Build up the JSON data
	size_t msg_len = strlen(message);
	char *data = static_cast<char *>(malloc(msg_len + 15));
	memcpy(data, R"({"content":")", 13);
	memcpy(data+12, message, msg_len+1);
	memcpy(data+msg_len+12, R"("})", 3);
	ESP_LOGD(TAG, "Data Content: %s", data);

	// Set up and perform the request
	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_header(client, "Content-Type", "application/json");
	esp_http_client_set_post_field(client, data, strlen(data));
	esp_err_t err = esp_http_client_perform(client);
	if(err == ESP_OK){
		ESP_LOGI(TAG, "Webhook: %d", esp_http_client_get_status_code(client));
	}else{
		ESP_LOGE(TAG, "Webhook error: %s", esp_err_to_name(err));
	}

	// Cleanup the allocated memory and the client connection.
	esp_http_client_cleanup(client);
	free(data);
	return err;
}
