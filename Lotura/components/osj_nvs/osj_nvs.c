#include "osj_nvs.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "osj_config.h"

SystemConfig sys_config;
static SemaphoreHandle_t config_mutex = NULL;

void osj_config_lock(void) {
    if (config_mutex) xSemaphoreTake(config_mutex, portMAX_DELAY);
}

void osj_config_unlock(void) {
    if (config_mutex) xSemaphoreGive(config_mutex);
}


static const char *NAMESPACE = "storage";

void osj_nvs_init(void) {
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
		ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
    
    config_mutex = xSemaphoreCreateMutex();
    
    osj_config_lock();
    osj_nvs_get_str("apSsid", sys_config.apSsid, sizeof(sys_config.apSsid), "");
    osj_nvs_get_str("apPasswd", sys_config.apPasswd, sizeof(sys_config.apPasswd), "");
    osj_nvs_get_str("roomNo", sys_config.roomNo, sizeof(sys_config.roomNo), "0");
    osj_nvs_get_str("authId", sys_config.authId, sizeof(sys_config.authId), "");
    osj_nvs_get_str("authPasswd", sys_config.authPasswd, sizeof(sys_config.authPasswd), "");
    
    osj_nvs_get_str("ch1DeviceNo", sys_config.ch1DeviceNo, sizeof(sys_config.ch1DeviceNo), "1");
    osj_nvs_get_str("ch2DeviceNo", sys_config.ch2DeviceNo, sizeof(sys_config.ch2DeviceNo), "2");
    
    sys_config.ch1CurrW = osj_nvs_get_float("ch1CurrW", 0.2);
    sys_config.ch2CurrW = osj_nvs_get_float("ch2CurrW", 0.2);
    sys_config.ch1FlowW = osj_nvs_get_uint("ch1FlowW", 50);
    sys_config.ch2FlowW = osj_nvs_get_uint("ch2FlowW", 50);
    sys_config.ch1CurrD = osj_nvs_get_float("ch1CurrD", 0.5);
    sys_config.ch2CurrD = osj_nvs_get_float("ch2CurrD", 0.5);
    
    sys_config.ch1EndDelayW = osj_nvs_get_uint("ch1EndDelayW", 100000); // Correct default
    sys_config.ch2EndDelayW = osj_nvs_get_uint("ch2EndDelayW", 100000);
    sys_config.ch1EndDelayD = osj_nvs_get_uint("ch1EndDelayD", 10000);
    sys_config.ch2EndDelayD = osj_nvs_get_uint("ch2EndDelayD", 10000);
    
    sys_config.isCh1Live = osj_nvs_get_bool("isCh1Live", true);
    sys_config.isCh2Live = osj_nvs_get_bool("isCh2Live", true);
    
    sys_config.hysteresisMargin = osj_nvs_get_float("hysteresisMargin", 0.05f);
    osj_config_unlock();
}

void osj_nvs_set_str(const char *key, const char *value) {
	nvs_handle_t my_handle;
	esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK)
		return;

	err = nvs_set_str(my_handle, key, value);
	if (err == ESP_OK) {
		nvs_commit(my_handle);
        // Cache update
        osj_config_lock();
        if (strcmp(key, "apSsid") == 0) strncpy(sys_config.apSsid, value, sizeof(sys_config.apSsid));
        else if (strcmp(key, "apPasswd") == 0) strncpy(sys_config.apPasswd, value, sizeof(sys_config.apPasswd));
        else if (strcmp(key, "roomNo") == 0) strncpy(sys_config.roomNo, value, sizeof(sys_config.roomNo));
        else if (strcmp(key, "authId") == 0) strncpy(sys_config.authId, value, sizeof(sys_config.authId));
        else if (strcmp(key, "authPasswd") == 0) strncpy(sys_config.authPasswd, value, sizeof(sys_config.authPasswd));
        osj_config_unlock();
    }
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
	if (err == ESP_OK) {
		nvs_commit(my_handle);
        // Cache update
        osj_config_lock();
        if (strcmp(key, "ch1CurrW") == 0) sys_config.ch1CurrW = value;
        else if (strcmp(key, "ch2CurrW") == 0) sys_config.ch2CurrW = value;
        else if (strcmp(key, "ch1CurrD") == 0) sys_config.ch1CurrD = value;
        else if (strcmp(key, "ch2CurrD") == 0) sys_config.ch2CurrD = value;
        else if (strcmp(key, "hysteresisMargin") == 0) sys_config.hysteresisMargin = value;
        osj_config_unlock();
    }
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
	if (err == ESP_OK) {
		nvs_commit(my_handle);
        // Cache update
        osj_config_lock();
        if (strcmp(key, "ch1FlowW") == 0) sys_config.ch1FlowW = value;
        else if (strcmp(key, "ch2FlowW") == 0) sys_config.ch2FlowW = value;
        else if (strcmp(key, "ch1EndDelayW") == 0) sys_config.ch1EndDelayW = value;
        else if (strcmp(key, "ch2EndDelayW") == 0) sys_config.ch2EndDelayW = value;
        else if (strcmp(key, "ch1EndDelayD") == 0) sys_config.ch1EndDelayD = value;
        else if (strcmp(key, "ch2EndDelayD") == 0) sys_config.ch2EndDelayD = value;
        osj_config_unlock();
    }
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
	if (err == ESP_OK) {
		nvs_commit(my_handle);
        osj_config_lock();
        if (strcmp(key, "isCh1Live") == 0) sys_config.isCh1Live = value;
        else if (strcmp(key, "isCh2Live") == 0) sys_config.isCh2Live = value;
        osj_config_unlock();
    }
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