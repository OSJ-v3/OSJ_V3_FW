#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stdio.h>

#include "laundry_core.h"
#include "osj_gpio.h"
#include "osj_http.h"
#include "osj_nvs.h"
#include "osj_sensor.h"
#include "osj_websocket.h"
#include "osj_wifi.h"

static const char *TAG = "MAIN_APP";

void app_main(void) {
	ESP_LOGI(TAG, "[Step 1] Initializing System...");
	osj_nvs_init();
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	ESP_LOGI(TAG, "[Step 2] Initializing Peripherals...");
	osj_gpio_init();
	osj_sensor_init();

	ESP_LOGI(TAG, "[Step 3] Starting Network Services...");
	osj_wifi_init();
	osj_http_start_server();
	osj_websocket_start();

	ESP_LOGI(TAG, "[Step 4] Launching Core Logic Task...");
	xTaskCreate(laundry_core_task, "laundry_core", 6144, NULL, 5, NULL);
	ESP_LOGI(TAG, "System Boot Complete. Entering Idle Loop.");
}