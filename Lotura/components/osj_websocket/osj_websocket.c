#include "osj_websocket.h"
#include "cJSON.h"
#include "common_defs.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "mbedtls/base64.h"
#include "osj_nvs.h"
#include "osj_wifi.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "OSJ_WS";
static esp_websocket_client_handle_t client1 = NULL;
static esp_websocket_client_handle_t client2 = NULL;

static void websocket_event_handler(void *handler_args, esp_event_base_t base,
									int32_t event_id, void *event_data) {
	esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
	int client_num = (int)handler_args;

	switch (event_id) {
	case WEBSOCKET_EVENT_CONNECTED:
		ESP_LOGI(TAG, "Client %d: WEBSOCKET_EVENT_CONNECTED", client_num);
		break;
	case WEBSOCKET_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "Client %d: WEBSOCKET_EVENT_DISCONNECTED", client_num);
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
						ESP_LOGI(TAG, "Client %d: Received GetData request",
								 client_num);
					}
					cJSON_Delete(json);
				}
				free(payload);
			}
		}
		break;
	case WEBSOCKET_EVENT_ERROR:
		ESP_LOGI(TAG, "Client %d: WEBSOCKET_EVENT_ERROR", client_num);
		break;
	}
}

static esp_websocket_client_handle_t
start_client(int channel, const char *auth_b64, const char *room) {
	char headers[512];
	int device_id = (channel == 1) ? DEVICE_ID_CH_1 : DEVICE_ID_CH_2;

	snprintf(headers, sizeof(headers),
			 "Authorization: Basic %s\r\n"
			 "HWID: %d\r\n"
			 "ROOM: %s\r\n",
			 auth_b64, device_id, room);

	esp_websocket_client_config_t websocket_cfg = {};
	websocket_cfg.uri = "wss://lotura-prod.xquare.app/device";
	websocket_cfg.headers = headers;
	websocket_cfg.network_timeout_ms = 10000;
	websocket_cfg.ping_interval_sec = 10;

	esp_websocket_client_handle_t client =
		esp_websocket_client_init(&websocket_cfg);
	esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY,
								  websocket_event_handler, (void *)channel);
	esp_websocket_client_start(client);
	return client;
}

void osj_websocket_start(void) {
	ESP_LOGI(TAG, "Starting WebSocket Clients...");

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

	client1 = start_client(1, (char *)auth_b64, room);
	client2 = start_client(2, (char *)auth_b64, room);
}

void osj_websocket_send_status(int channel, int status,
							   const char *device_type) {
	esp_websocket_client_handle_t client = (channel == 1) ? client1 : client2;
	if (!client || !esp_websocket_client_is_connected(client))
		return;

	int device_id = (channel == 1) ? DEVICE_ID_CH_1 : DEVICE_ID_CH_2;

	cJSON *root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "id", device_id);
	cJSON_AddStringToObject(root, "device_type", device_type);
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
	esp_websocket_client_handle_t client = (channel == 1) ? client1 : client2;
	if (!client || !esp_websocket_client_is_connected(client))
		return;

	int device_id = (channel == 1) ? DEVICE_ID_CH_1 : DEVICE_ID_CH_2;

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "title", "Log");
	cJSON_AddNumberToObject(root, "id", device_id);

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