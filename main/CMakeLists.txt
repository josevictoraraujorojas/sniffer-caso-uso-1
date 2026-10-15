idf_component_register(SRCS "hello_world_main.c"
                       PRIV_REQUIRES spi_flash
                       REQUIRES esp_wifi nvs_flash esp_netif esp_timer driver
                       INCLUDE_DIRS "")