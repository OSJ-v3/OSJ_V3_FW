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
#include <stdlib.h>
#include <string.h>

static const char *TAG = "OSJ_HTTP";
static httpd_handle_t server = NULL;

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[] asm("_binary_index_html_end");

static void str_replace(char *target, const char *needle,
						const char *replacement) {
	char buffer[10240];
	char *insert_point = &buffer[0];
	const char *tmp = target;
	size_t needle_len = strlen(needle);
	size_t repl_len = strlen(replacement);

	while (1) {
		const char *p = strstr(tmp, needle);
		if (p == NULL) {
			strcpy(insert_point, tmp);
			break;
		}
		memcpy(insert_point, tmp, p - tmp);
		insert_point += p - tmp;
		memcpy(insert_point, replacement, repl_len);
		insert_point += repl_len;
		tmp = p + needle_len;
	}
	strcpy(target, buffer);
}

static esp_err_t root_get_handler(httpd_req_t *req) {
	char ssid[32], ip[16], mac[18], room[16];
	char ch1Dev[8], ch2Dev[8];
	float ch1CW, ch2CW, ch1CD, ch2CD;
	uint32_t ch1FW, ch2FW;
	uint32_t ch1EDW, ch2EDW, ch1EDD, ch2EDD;
	bool isCh1Live, isCh2Live;

	osj_nvs_get_str("apSsid", ssid, sizeof(ssid), "");
	osj_wifi_get_ip(ip);
	osj_wifi_get_mac(mac);
	osj_nvs_get_str("roomNo", room, sizeof(room), "0");
	int8_t rssi = osj_wifi_get_rssi();

	osj_nvs_get_str("ch1DeviceNo", ch1Dev, sizeof(ch1Dev), "1");
	osj_nvs_get_str("ch2DeviceNo", ch2Dev, sizeof(ch2Dev), "2");

	ch1CW = osj_nvs_get_float("ch1CurrW", 0.2);
	ch2CW = osj_nvs_get_float("ch2CurrW", 0.2);
	ch1FW = osj_nvs_get_uint("ch1FlowW", 50);
	ch2FW = osj_nvs_get_uint("ch2FlowW", 50);
	ch1CD = osj_nvs_get_float("ch1CurrD", 0.5);
	ch2CD = osj_nvs_get_float("ch2CurrD", 0.5);
	ch1EDW = osj_nvs_get_uint("ch1EndDelayW", 10);
	ch2EDW = osj_nvs_get_uint("ch2EndDelayW", 10);
	ch1EDD = osj_nvs_get_uint("ch1EndDelayD", 10);
	ch2EDD = osj_nvs_get_uint("ch2EndDelayD", 10);
	isCh1Live = osj_nvs_get_bool("isCh1Live", true);
	isCh2Live = osj_nvs_get_bool("isCh2Live", true);

	size_t html_len = index_html_end - index_html_start;
	char *resp_str = malloc(20000);
	if (!resp_str)
		return ESP_FAIL;

	memcpy(resp_str, index_html_start, html_len);
	resp_str[html_len] = '\0';

	char temp_val[64];

	str_replace(resp_str, "%deviceName%", "OSJ Device");
	str_replace(resp_str, "%apSsid%", ssid);

	snprintf(temp_val, sizeof(temp_val), "%d", rssi);
	str_replace(resp_str, "%wifiRssi%", temp_val);
	str_replace(resp_str, "%wifiQuality%", (rssi > -50) ? "Good" : "Weak");

	str_replace(resp_str, "%wifiIp%", ip);
	str_replace(resp_str, "%mac%", mac);
	str_replace(resp_str, "%roomNo%", room);

	snprintf(temp_val, sizeof(temp_val), "%lu",
			 esp_get_free_heap_size() / 1024);
	str_replace(resp_str, "%heap%", temp_val);

	str_replace(resp_str, "%ch1DeviceNo%", ch1Dev);
	str_replace(resp_str, "%isCh1Live%", isCh1Live ? "Yes" : "No");
	str_replace(resp_str, "%ch1Mode%",
				!FAST_GPIO_READ(PIN_CH1_MODE) ? "Wash" : "Dry");

	snprintf(temp_val, sizeof(temp_val), "%.2f", ch1CW);
	str_replace(resp_str, "%ch1CurrW%", temp_val);
	snprintf(temp_val, sizeof(temp_val), "%lu", ch1FW);
	str_replace(resp_str, "%ch1FlowW%", temp_val);
	snprintf(temp_val, sizeof(temp_val), "%.2f", ch1CD);
	str_replace(resp_str, "%ch1CurrD%", temp_val);

	snprintf(temp_val, sizeof(temp_val), "%lu", ch1EDW);
	str_replace(resp_str, "%ch1EndDelayW%", temp_val);
	snprintf(temp_val, sizeof(temp_val), "%lu", ch1EDD);
	str_replace(resp_str, "%ch1EndDelayD%", temp_val);

	snprintf(temp_val, sizeof(temp_val), "%.2f", osj_sensor_get_rms(1));
	str_replace(resp_str, "%ampsTrms1%", temp_val);
	snprintf(temp_val, sizeof(temp_val), "%d", osj_sensor_get_drain(1));
	str_replace(resp_str, "%waterSensorData1%", temp_val);
	snprintf(temp_val, sizeof(temp_val), "%lu",
			 (osj_sensor_get_flow(1) * 60) / 7);
	str_replace(resp_str, "%lHour1%", temp_val);

	str_replace(resp_str, "%ch2DeviceNo%", ch2Dev);
	str_replace(resp_str, "%isCh2Live%", isCh2Live ? "Yes" : "No");
	str_replace(resp_str, "%ch2Mode%",
				!FAST_GPIO_READ(PIN_CH2_MODE) ? "Wash" : "Dry");

	snprintf(temp_val, sizeof(temp_val), "%.2f", ch2CW);
	str_replace(resp_str, "%ch2CurrW%", temp_val);
	snprintf(temp_val, sizeof(temp_val), "%lu", ch2FW);
	str_replace(resp_str, "%ch2FlowW%", temp_val);
	snprintf(temp_val, sizeof(temp_val), "%.2f", ch2CD);
	str_replace(resp_str, "%ch2CurrD%", temp_val);

	snprintf(temp_val, sizeof(temp_val), "%lu", ch2EDW);
	str_replace(resp_str, "%ch2EndDelayW%", temp_val);
	snprintf(temp_val, sizeof(temp_val), "%lu", ch2EDD);
	str_replace(resp_str, "%ch2EndDelayD%", temp_val);

	snprintf(temp_val, sizeof(temp_val), "%.2f", osj_sensor_get_rms(2));
	str_replace(resp_str, "%ampsTrms2%", temp_val);
	snprintf(temp_val, sizeof(temp_val), "%d", osj_sensor_get_drain(2));
	str_replace(resp_str, "%waterSensorData2%", temp_val);
	snprintf(temp_val, sizeof(temp_val), "%lu",
			 (osj_sensor_get_flow(2) * 60) / 7);
	str_replace(resp_str, "%lHour2%", temp_val);

	str_replace(resp_str, "%flashSize%", "4096");
	str_replace(resp_str, "%buildVer%", "v1.0");

	httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
	free(resp_str);
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
	httpd_resp_send(req, "OTA Not Implemented", HTTPD_RESP_USE_STRLEN);
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