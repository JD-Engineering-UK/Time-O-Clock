idf_component_register(
        SRCS "webhook.cpp"
        INCLUDE_DIRS "include"
        PRIV_REQUIRES "esp_http_client" "esp-tls"
)