#include "osj_nvs.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>

// static const char *TAG = "OSJ_NVS";
static const char *NAMESPACE = "storage";

void osj_nvs_init(void) {
	// Initialize NVS partition
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
		ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
}

void osj_nvs_set_str(const char *key, const char *value) {
	nvs_handle_t my_handle;
	esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
		return;

	err = nvs_set_str(my_handle, key, value);
	if (err == ESP_OK)
		nvs_commit(my_handle);
	nvs_close(my_handle);
}

void osj_nvs_get_str(const char *key, char *out_value, size_t max_len,
					 const char *default_value) {
	nvs_handle_t my_handle;
	esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &my_handle);
	if (err != ESP_OK) {
		snprintf(out_value, max_len, "%s", default_value);
		return;
	}

	size_t required_size;
	err = nvs_get_str(my_handle, key, NULL, &required_size);
	if (err == ESP_OK && required_size <= max_len) {
		nvs_get_str(my_handle, key, out_value, &required_size);
	} else {
		snprintf(out_value, max_len, "%s", default_value);
	}
	nvs_close(my_handle);
}

void osj_nvs_set_float(const char *key, float value) {
	nvs_handle_t my_handle;
	esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
		return;

	// Store float as blob
	err = nvs_set_blob(my_handle, key, &value, sizeof(float));
	if (err == ESP_OK)
		nvs_commit(my_handle);
	nvs_close(my_handle);
}

float osj_nvs_get_float(const char *key, float default_value) {
	nvs_handle_t my_handle;
	esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &my_handle);
	if (err != ESP_OK)
		return default_value;

	float value = 0.0f;
	size_t size = sizeof(float);
	err = nvs_get_blob(my_handle, key, &value, &size);
	nvs_close(my_handle);

	if (err == ESP_OK && size == sizeof(float)) {
		return value;
	}
	return default_value;
}

void osj_nvs_set_uint(const char *key, uint32_t value) {
	nvs_handle_t my_handle;
	esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
		return;

	err = nvs_set_u32(my_handle, key, value);
	if (err == ESP_OK)
		nvs_commit(my_handle);
	nvs_close(my_handle);
}

uint32_t osj_nvs_get_uint(const char *key, uint32_t default_value) {
	nvs_handle_t my_handle;
	esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &my_handle);
	if (err != ESP_OK)
		return default_value;

	uint32_t value = 0;
	err = nvs_get_u32(my_handle, key, &value);
	nvs_close(my_handle);

	if (err == ESP_OK) {
		return value;
	}
	return default_value;
}

void osj_nvs_set_bool(const char *key, bool value) {
	nvs_handle_t my_handle;
	esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
		return;

	err = nvs_set_u8(my_handle, key, value ? 1 : 0);
	if (err == ESP_OK)
		nvs_commit(my_handle);
	nvs_close(my_handle);
}

bool osj_nvs_get_bool(const char *key, bool default_value) {
	nvs_handle_t my_handle;
	esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &my_handle);
	if (err != ESP_OK)
		return default_value;

	uint8_t value = 0;
	err = nvs_get_u8(my_handle, key, &value);
	nvs_close(my_handle);

	if (err == ESP_OK) {
		return value != 0;
	}
	return default_value;
}