#include "osj_http.h"
#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "gpio_definitions.h"
#include "osj_gpio.h"
#include "osj_nvs.h"
#include "osj_sensor.h"
#include "osj_wifi.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "OSJ_HTTP";
static httpd_handle_t server = NULL;

#include "osj_config.h"

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[] asm("_binary_index_html_end");

#include "laundry_core.h"

// Helper to send a chunk of data
static esp_err_t send_chunk(httpd_req_t *req, const char *str) {
	return httpd_resp_send_chunk(req, str, strlen(str));
}

static esp_err_t root_get_handler(httpd_req_t *req) {
	char ssid[32], ip[16], mac[18], room[16];
	char ch1Dev[8], ch2Dev[8];
	float ch1CW, ch2CW, ch1CD, ch2CD;
	uint32_t ch1FW, ch2FW;
	uint32_t ch1EDW, ch2EDW, ch1EDD, ch2EDD;
	bool isCh1Live, isCh2Live;
	char temp_val[64];

	osj_wifi_get_ip(ip);
	osj_wifi_get_mac(mac);
	int8_t rssi = osj_wifi_get_rssi();

    // Use sys_config
    osj_config_lock();
	strncpy(ssid, sys_config.apSsid, sizeof(ssid));
	strncpy(room, sys_config.roomNo, sizeof(room));
	
	strncpy(ch1Dev, sys_config.ch1DeviceNo, sizeof(ch1Dev));
	strncpy(ch2Dev, sys_config.ch2DeviceNo, sizeof(ch2Dev));

	ch1CW = sys_config.ch1CurrW;
	ch2CW = sys_config.ch2CurrW;
	ch1FW = sys_config.ch1FlowW;
	ch2FW = sys_config.ch2FlowW;
	ch1CD = sys_config.ch1CurrD;
	ch2CD = sys_config.ch2CurrD;
	ch1EDW = sys_config.ch1EndDelayW;
	ch2EDW = sys_config.ch2EndDelayW;
	ch1EDD = sys_config.ch1EndDelayD;
	ch2EDD = sys_config.ch2EndDelayD;
	isCh1Live = sys_config.isCh1Live;
	isCh2Live = sys_config.isCh2Live;
    osj_config_unlock();

	const char *ptr = index_html_start;
	const char *end = index_html_end;
	
	while (ptr < end) {
	    const char *token_start = strchr(ptr, '%');
	    
	    if (token_start == NULL || token_start >= end) {
	        httpd_resp_send_chunk(req, ptr, end - ptr);
	        break;
	    }
	    
	    if (token_start > ptr) {
	        httpd_resp_send_chunk(req, ptr, token_start - ptr);
	    }
	    
	    const char *token_end = strchr(token_start + 1, '%');
	    if (token_end == NULL || token_end >= end) {
	        httpd_resp_send_chunk(req, token_start, end - token_start);
	        break;
	    }
	    
	    int token_len = token_end - token_start - 1;
	    if (token_len <= 0) {
            send_chunk(req, "%");
            ptr = token_end + 1;
            continue;
	    }

        bool matched = false;
        
        #define IS_TOKEN(s) (strncmp(token_start + 1, s, token_len) == 0 && strlen(s) == token_len)
        
        if (IS_TOKEN("deviceName")) {
            send_chunk(req, "OSJ Device"); matched = true;
        } else if (IS_TOKEN("apSsid")) {
            send_chunk(req, ssid); matched = true;
        } else if (IS_TOKEN("wifiRssi")) {
            snprintf(temp_val, sizeof(temp_val), "%d", rssi);
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("wifiQuality")) {
            send_chunk(req, (rssi > -50) ? "Good" : "Weak"); matched = true;
        } else if (IS_TOKEN("wifiIp")) {
            send_chunk(req, ip); matched = true;
        } else if (IS_TOKEN("mac")) {
            send_chunk(req, mac); matched = true;
        } else if (IS_TOKEN("roomNo")) {
            send_chunk(req, room); matched = true;
        } else if (IS_TOKEN("heap")) {
            snprintf(temp_val, sizeof(temp_val), "%lu", esp_get_free_heap_size() / 1024);
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("ch1DeviceNo")) {
            send_chunk(req, ch1Dev); matched = true;
        } else if (IS_TOKEN("isCh1Live")) {
            send_chunk(req, isCh1Live ? "Yes" : "No"); matched = true;
        } else if (IS_TOKEN("ch1Mode")) {
            send_chunk(req, !FAST_GPIO_READ(PIN_CH1_MODE) ? "Wash" : "Dry"); matched = true;
        } else if (IS_TOKEN("ch1CurrW")) {
            snprintf(temp_val, sizeof(temp_val), "%.2f", ch1CW);
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("ch1FlowW")) {
            snprintf(temp_val, sizeof(temp_val), "%lu", ch1FW);
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("ch1CurrD")) {
            snprintf(temp_val, sizeof(temp_val), "%.2f", ch1CD);
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("ch1EndDelayW")) {
            snprintf(temp_val, sizeof(temp_val), "%lu", ch1EDW);
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("ch1EndDelayD")) {
            snprintf(temp_val, sizeof(temp_val), "%lu", ch1EDD);
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("ampsTrms1")) {
            snprintf(temp_val, sizeof(temp_val), "%.2f", osj_sensor_get_rms(1));
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("waterSensorData1")) {
            snprintf(temp_val, sizeof(temp_val), "%d", osj_sensor_get_drain(1));
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("lHour1")) {
            snprintf(temp_val, sizeof(temp_val), "%lu", laundry_core_get_lHour(1));
            send_chunk(req, temp_val); matched = true;
        }
        else if (IS_TOKEN("ch2DeviceNo")) {
            send_chunk(req, ch2Dev); matched = true;
        } else if (IS_TOKEN("isCh2Live")) {
            send_chunk(req, isCh2Live ? "Yes" : "No"); matched = true;
        } else if (IS_TOKEN("ch2Mode")) {
            send_chunk(req, !FAST_GPIO_READ(PIN_CH2_MODE) ? "Wash" : "Dry"); matched = true;
        } else if (IS_TOKEN("ch2CurrW")) {
            snprintf(temp_val, sizeof(temp_val), "%.2f", ch2CW);
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("ch2FlowW")) {
            snprintf(temp_val, sizeof(temp_val), "%lu", ch2FW);
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("ch2CurrD")) {
            snprintf(temp_val, sizeof(temp_val), "%.2f", ch2CD);
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("ch2EndDelayW")) {
            snprintf(temp_val, sizeof(temp_val), "%lu", ch2EDW);
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("ch2EndDelayD")) {
            snprintf(temp_val, sizeof(temp_val), "%lu", ch2EDD);
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("ampsTrms2")) {
            snprintf(temp_val, sizeof(temp_val), "%.2f", osj_sensor_get_rms(2));
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("waterSensorData2")) {
            snprintf(temp_val, sizeof(temp_val), "%d", osj_sensor_get_drain(2));
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("lHour2")) {
            snprintf(temp_val, sizeof(temp_val), "%lu", laundry_core_get_lHour(2));
            send_chunk(req, temp_val); matched = true;
        } else if (IS_TOKEN("flashSize")) {
            send_chunk(req, "4096"); matched = true;
        } else if (IS_TOKEN("buildVer")) {
            send_chunk(req, "v1.0"); matched = true;
        }

        if (!matched) {
            httpd_resp_send_chunk(req, token_start, token_len + 2);
        }
        
        ptr = token_end + 1;
	}
	
	httpd_resp_send_chunk(req, NULL, 0);
	return ESP_OK;
}

static esp_err_t wifi_post_handler(httpd_req_t *req) {
	char buf[100];
	int ret, remaining = req->content_len;
	if (remaining >= sizeof(buf)) {
		httpd_resp_send_500(req);
		return ESP_FAIL;
	}
	ret = httpd_req_recv(req, buf, remaining);
	if (ret <= 0) {
		if (ret == HTTPD_SOCK_ERR_TIMEOUT)
			httpd_resp_send_408(req);
		return ESP_FAIL;
	}
	buf[ret] = '\0';

	char *ssid_ptr = strstr(buf, "wifiSsid=");
	char *pass_ptr = strstr(buf, "wifiPass=");

	if (ssid_ptr) {
		ssid_ptr += 9;
		char *end = strchr(ssid_ptr, '&');
		if (end)
			*end = '\0';
		osj_nvs_set_str("apSsid", ssid_ptr);
	}
	if (pass_ptr) {
		pass_ptr += 9;
		char *end = strchr(pass_ptr, '&');
		if (end)
			*end = '\0';
		osj_nvs_set_str("apPasswd", pass_ptr);
	}

	httpd_resp_set_status(req, "303 See Other");
	httpd_resp_set_hdr(req, "Location", "/");
	httpd_resp_send(req, NULL, 0);
	return ESP_OK;
}

static esp_err_t room_post_handler(httpd_req_t *req) {
	char buf[100];
	int ret, remaining = req->content_len;
	if (remaining >= sizeof(buf)) {
		httpd_resp_send_500(req);
		return ESP_FAIL;
	}
	ret = httpd_req_recv(req, buf, remaining);
	if (ret <= 0)
		return ESP_FAIL;
	buf[ret] = '\0';

	char *room_ptr = strstr(buf, "roomNo=");
	if (room_ptr) {
		room_ptr += 7;
		char *end = strchr(room_ptr, '&');
		if (end)
			*end = '\0';
		osj_nvs_set_str("roomNo", room_ptr);
	}

	httpd_resp_set_status(req, "303 See Other");
	httpd_resp_set_hdr(req, "Location", "/");
	httpd_resp_send(req, NULL, 0);
	return ESP_OK;
}

static esp_err_t reboot_get_handler(httpd_req_t *req) {
	esp_restart();
	return ESP_OK;
}

static esp_err_t ch1_post_handler(httpd_req_t *req) {
	httpd_resp_set_status(req, "303 See Other");
	httpd_resp_set_hdr(req, "Location", "/");
	httpd_resp_send(req, NULL, 0);
	return ESP_OK;
}

static esp_err_t ch2_post_handler(httpd_req_t *req) {
	httpd_resp_set_status(req, "303 See Other");
	httpd_resp_set_hdr(req, "Location", "/");
	httpd_resp_send(req, NULL, 0);
	return ESP_OK;
}

static esp_err_t update_post_handler(httpd_req_t *req) {
    char buf[1024];
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;
    esp_err_t err;

    ESP_LOGI(TAG, "Starting OTA Update...");

    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "No update partition found");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int received;
    int total_received = 0;
    while ((received = httpd_req_recv(req, buf, sizeof(buf))) > 0) {
        err = esp_ota_write(update_handle, buf, received);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
            esp_ota_end(update_handle);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        total_received += received;
    }

    if (received < 0 && received != HTTPD_SOCK_ERR_TIMEOUT) {
        ESP_LOGE(TAG, "httpd_req_recv failed");
        esp_ota_end(update_handle);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed (%s)", esp_err_to_name(err));
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)", esp_err_to_name(err));
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "OTA Update Success! Rebooting...");
    httpd_resp_send(req, "Update Success. Rebooting...", HTTPD_RESP_USE_STRLEN);
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
    return ESP_OK;
}

static esp_err_t set_default_val_handler(httpd_req_t *req) {
	httpd_resp_set_status(req, "303 See Other");
	httpd_resp_set_hdr(req, "Location", "/");
	httpd_resp_send(req, NULL, 0);
	return ESP_OK;
}

void osj_http_start_server(void) {
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.stack_size = 8192;
	config.max_uri_handlers = 10;

	if (httpd_start(&server, &config) == ESP_OK) {
		httpd_uri_t root_uri = {.uri = "/",
								.method = HTTP_GET,
								.handler = root_get_handler,
								.user_ctx = NULL};
		httpd_register_uri_handler(server, &root_uri);

		httpd_uri_t wifi_uri = {.uri = "/wifi",
								.method = HTTP_POST,
								.handler = wifi_post_handler,
								.user_ctx = NULL};
		httpd_register_uri_handler(server, &wifi_uri);

		httpd_uri_t room_uri = {.uri = "/roomno",
								.method = HTTP_POST,
								.handler = room_post_handler,
								.user_ctx = NULL};
		httpd_register_uri_handler(server, &room_uri);

		httpd_uri_t reboot_uri = {.uri = "/reboot",
								  .method = HTTP_GET,
								  .handler = reboot_get_handler,
								  .user_ctx = NULL};
		httpd_register_uri_handler(server, &reboot_uri);

		httpd_uri_t ch1_uri = {.uri = "/CH1",
							   .method = HTTP_POST,
							   .handler = ch1_post_handler,
							   .user_ctx = NULL};
		httpd_register_uri_handler(server, &ch1_uri);

		httpd_uri_t ch2_uri = {.uri = "/CH2",
							   .method = HTTP_POST,
							   .handler = ch2_post_handler,
							   .user_ctx = NULL};
		httpd_register_uri_handler(server, &ch2_uri);

		httpd_uri_t update_uri = {.uri = "/update",
								  .method = HTTP_POST,
								  .handler = update_post_handler,
								  .user_ctx = NULL};
		httpd_register_uri_handler(server, &update_uri);

		httpd_uri_t default_uri = {.uri = "/SetDefaultVal",
								   .method = HTTP_GET,
								   .handler = set_default_val_handler,
								   .user_ctx = NULL};
		httpd_register_uri_handler(server, &default_uri);

		ESP_LOGI(TAG, "HTTP Server Started");
	}
}