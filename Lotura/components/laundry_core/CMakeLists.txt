idf_component_register(SRCS "laundry_core.c"
                       INCLUDE_DIRS "include"
                       REQUIRES osj_sensor osj_websocket osj_nvs osj_gpio json)
