idf_component_register(
        SRCS "wifi.cpp" "credentials.cpp"
        INCLUDE_DIRS "include"
        REQUIRES "esp_wifi" "files"
        EMBED_FILES "../../wifi_deets.bin"
)