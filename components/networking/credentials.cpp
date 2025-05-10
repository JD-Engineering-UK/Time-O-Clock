#include "credentials.h"
#include <malloc.h>
#include <string.h>
#include "esp_log.h"


const static char *TAG = __FILENAME__;

// First byte of the credentials file is the number of credentials
const uint8_t credentials_count = wifi_deets_bin_start[0];
static cred_t* credentials;
static bool ready = false;

void init_credentials()
{
    if(ready)
    {   // Stop repeated calls from reallocating memory and reinitialising the structs
        return;
    }
    // Allocate space for the details structs
    credentials = (cred_t *)malloc(credentials_count * sizeof(cred_t));

    uint16_t file_idx = 1;
    for(uint8_t idx = 0; idx < credentials_count; ++idx)
    {
        bool has_password = wifi_deets_bin_start[file_idx++];
        credentials[idx].ssid_len = wifi_deets_bin_start[file_idx++];
        credentials[idx].ssid = wifi_deets_bin_start + file_idx;
        file_idx += credentials[idx].ssid_len;

        if(!has_password)
        {
            credentials[idx].passwd_len = 0;
            credentials[idx].password = nullptr;
        }
        else
        {
            credentials[idx].passwd_len = wifi_deets_bin_start[file_idx++];
            credentials[idx].password = wifi_deets_bin_start + file_idx;
            file_idx += credentials[idx].passwd_len;
        }
    }
    ready = true;
}

cred_t* get_credentials(uint8_t *ssid, uint8_t ssid_len)
{   // Search for the credentials for the network with the ssid provided
    if(!ready) return nullptr;
    uint8_t temp_str[50] = {0};

    for(int idx=0; idx < credentials_count; ++idx)
    {
        cred_t *cred = credentials+idx;
        memset(temp_str, 0, 50);
        memcpy(temp_str, cred->ssid, cred->ssid_len);
        if(cred->ssid_len > ssid_len) continue;
        ESP_LOGD(TAG, "NW passed sanity check (%s)", temp_str);
        

        if(memcmp(ssid, cred->ssid, cred->ssid_len) == 0)
        {
            ESP_LOGD(TAG, "Found network!");
            return cred;
        }
    }
    ESP_LOGD(TAG, "Checked full list of known networks");
    return nullptr;
}
