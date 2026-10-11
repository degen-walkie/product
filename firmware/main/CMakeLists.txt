idf_component_register(
    SRCS main.c board.c platform.c
    INCLUDE_DIRS .
    REQUIRES dw_core driver esp_lcd esp_timer esp_wifi esp_netif esp_event nvs_flash esp_http_client mbedtls)
