idf_component_register(SRCS "osj_websocket.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_websocket_client osj_nvs osj_wifi json)
