#pragma once

#include "files.h"
#include <cstdint> // For uint8_t

EMBEDDED_FILE(wifi_deets_bin);


typedef struct credentials_tag {
    const uint8_t *ssid;
    uint8_t ssid_len;
    const uint8_t *password;
    uint8_t passwd_len;
} cred_t;



void init_credentials();
cred_t* get_credentials(uint8_t *ssid, uint8_t ssid_len);