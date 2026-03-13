#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_ota_ops.h"
#include <stdio.h>

#include "laundry_core.h"
#include "osj_gpio.h"
#include "osj_http.h"
#include "osj_nvs.h"
#include "osj_sensor.h"
#include "osj_websocket.h"
#include "osj_wifi.h"

#include "esp_sntp.h"
#include <time.h>

static const char *TAG = "MAIN_APP";

void app_main(void) {
	ESP_LOGI(TAG, "[Step 1] Initializing System...");
	osj_nvs_init();
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	// OTA Rollback check (Mark as valid after booting)
	esp_ota_mark_app_valid_cancel_rollback();

	ESP_LOGI(TAG, "[Step 2] Initializing Peripherals...");
	osj_gpio_init();
	osj_sensor_init();

	ESP_LOGI(TAG, "[Step 3] Starting Network Services...");
	osj_wifi_init();
	osj_http_start_server();

	int retry = 0;
	const int retry_count = 15;
	ESP_LOGI(TAG, "Waiting for system time to be synchronized...");
	while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
		ESP_LOGI(TAG, "Waiting... (%d/%d)", retry, retry_count);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}

	if (retry == retry_count) {
		ESP_LOGE(TAG, "Failed to synchronize time. WSS connection might fail.");
	} else {
		ESP_LOGI(TAG, "Time synchronized successfully!");
	}

	osj_websocket_start();

	ESP_LOGI(TAG, "[Step 4] Launching Core Logic Task...");
	xTaskCreate(laundry_core_task, "laundry_core", 6144, NULL, 5, NULL);
	ESP_LOGI(TAG, "System Boot Complete. Entering Idle Loop.");
}