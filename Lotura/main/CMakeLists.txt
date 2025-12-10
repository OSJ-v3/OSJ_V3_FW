idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "include"
                       REQUIRES osj_gpio osj_sensor osj_wifi osj_http osj_websocket osj_nvs laundry_core osj_common)
