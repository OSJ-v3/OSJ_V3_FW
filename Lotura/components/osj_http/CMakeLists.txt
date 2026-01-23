idf_component_register(SRCS "osj_http.c"
                       INCLUDE_DIRS "include"
                       EMBED_TXTFILES "html/index.html"
                       REQUIRES esp_http_server osj_nvs osj_sensor osj_wifi osj_gpio json osj_common laundry_core app_update esp_partition)
