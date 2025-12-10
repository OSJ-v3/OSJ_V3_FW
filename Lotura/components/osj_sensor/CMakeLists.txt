idf_component_register(SRCS "osj_sensor.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_adc driver osj_gpio)
