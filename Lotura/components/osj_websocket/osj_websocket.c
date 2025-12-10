#include "osj_websocket.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "mbedtls/base64.h"
#include "osj_nvs.h"
#include "osj_wifi.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "OSJ_WS";
static esp_websocket_client_handle_t client = NULL;

static void websocket_event_handler(void *handler_args, esp_event_base_t base,
									int32_t event_id, void *event_data) {
	esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
	switch (event_id) {
	case WEBSOCKET_EVENT_CONNECTED:
		ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
		break;
	case WEBSOCKET_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
		break;
	case WEBSOCKET_EVENT_DATA:
		if (data->op_code == WS_TRANSPORT_OPCODES_TEXT) {
			char *payload = strndup(data->data_ptr, data->data_len);
			if (payload) {
				cJSON *json = cJSON_Parse(payload);
				if (json) {
					cJSON *title = cJSON_GetObjectItem(json, "title");
					if (cJSON_IsString(title) &&
						strcmp(title->valuestring, "GetData") == 0) {
						ESP_LOGI(TAG, "Received GetData request");
					}
					cJSON_Delete(json);
				}
				free(payload);
			}
		}
		break;
	case WEBSOCKET_EVENT_ERROR:
		ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
		break;
	}
}

void osj_websocket_start(void) {
	ESP_LOGI(TAG, "Starting WebSocket Client...");

	char auth_id[32], auth_pass[32], room[16];
	osj_nvs_get_str("authId", auth_id, sizeof(auth_id), "");
	osj_nvs_get_str("authPasswd", auth_pass, sizeof(auth_pass), "");
	osj_nvs_get_str("roomNo", room, sizeof(room), "0");

	char auth_str[65];
	snprintf(auth_str, sizeof(auth_str), "%s:%s", auth_id, auth_pass);
	unsigned char auth_b64[100];
	size_t out_len;
	mbedtls_base64_encode(auth_b64, sizeof(auth_b64), &out_len,
						  (unsigned char *)auth_str, strlen(auth_str));

	char headers[512];
	snprintf(headers, sizeof(headers),
			 "Authorization: Basic %s\r\n"
			 "HWID: 0\r\n"
			 "CH1: 1\r\n"
			 "CH2: 2\r\n"
			 "ROOM: %s\r\n",
			 auth_b64, room);

	esp_websocket_client_config_t websocket_cfg = {};
	websocket_cfg.uri = "wss://lotura-prod.xquare.app/device";
	websocket_cfg.headers = headers;
	websocket_cfg.network_timeout_ms = 10000;
	websocket_cfg.ping_interval_sec = 10;

	client = esp_websocket_client_init(&websocket_cfg);
	esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY,
								  websocket_event_handler, (void *)client);
	esp_websocket_client_start(client);
}

void osj_websocket_send_status(int channel, int status) {
	if (!client || !esp_websocket_client_is_connected(client))
		return;

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "title", "Update");
	cJSON_AddStringToObject(root, "id", (channel == 1) ? "1" : "2");
	cJSON_AddNumberToObject(root, "type", 0);
	cJSON_AddNumberToObject(root, "state", status);

	char *json_str = cJSON_PrintUnformatted(root);
	if (json_str) {
		esp_websocket_client_send_text(client, json_str, strlen(json_str),
									   portMAX_DELAY);
		free(json_str);
	}
	cJSON_Delete(root);
}

void osj_websocket_send_log(int channel, const char *log_json) {
	if (!client || !esp_websocket_client_is_connected(client))
		return;

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "title", "Log");
	cJSON_AddStringToObject(root, "id", (channel == 1) ? "1" : "2");

	cJSON *log_obj = cJSON_Parse(log_json);
	if (log_obj) {
		cJSON_AddItemToObject(root, "log", log_obj);
	} else {
		cJSON_AddStringToObject(root, "log", "{}");
	}

	char *json_str = cJSON_PrintUnformatted(root);
	if (json_str) {
		esp_websocket_client_send_text(client, json_str, strlen(json_str),
									   portMAX_DELAY);
		free(json_str);
	}
	cJSON_Delete(root);
}