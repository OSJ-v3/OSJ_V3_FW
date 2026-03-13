#include "osj_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "osj_nvs.h"
#include <string.h>
#include "esp_sntp.h"

static const char *TAG = "OSJ_WIFI";
static bool s_is_connected = false;
static int s_retry_num = 0;
static TimerHandle_t s_retry_timer = NULL;

static void retry_timer_callback(TimerHandle_t xTimer) {
    ESP_LOGI(TAG, "Retry timer expired, connecting to AP...");
    s_retry_num = 0;
    esp_wifi_connect();
}

#define DEFAULT_SSID "OSJ_WIFI"
#define DEFAULT_PASS "12345678"

static void start_softap(void) {
    wifi_mode_t current_mode;
    esp_wifi_get_mode(&current_mode);
    if (current_mode == WIFI_MODE_APSTA || current_mode == WIFI_MODE_AP) {
        return; // Already in AP mode
    }

    ESP_LOGW(TAG, "Enabling SoftAP fallback for provisioning...");
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    
    wifi_config_t ap_config = {
        .ap = {
            .ssid = DEFAULT_SSID,
            .ssid_len = strlen(DEFAULT_SSID),
            .channel = 1,
            .password = DEFAULT_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
}

static void event_handler(void *arg, esp_event_base_t event_base,
						  int32_t event_id, void *event_data) {
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        wifi_config_t config;
        esp_wifi_get_config(WIFI_IF_STA, &config);
        if (strlen((char*)config.sta.ssid) > 0) {
		    esp_wifi_connect();
        }
	} else if (event_base == WIFI_EVENT &&
			   event_id == WIFI_EVENT_STA_DISCONNECTED) {
		s_is_connected = false;
		if (s_retry_num < 5) {
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		} else {
			ESP_LOGW(TAG, "connect to the AP fail, starting SoftAP & retry timer");
            start_softap();
            if (s_retry_timer != NULL) {
                xTimerStart(s_retry_timer, 0);
            }
		}
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		s_is_connected = true;

		static bool sntp_inited = false;
		if (!sntp_inited) {
			ESP_LOGI(TAG, "Initializing SNTP for WSS certificate validation");
			esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
			esp_sntp_setservername(0, "pool.ntp.org");
			esp_sntp_init();
			sntp_inited = true;
		}
	}
}

void osj_wifi_init(void) {
	ESP_LOGI(TAG, "Initializing WiFi...");

    s_retry_timer = xTimerCreate("wifi_retry", pdMS_TO_TICKS(5000),
                                 pdFALSE, (void *)0, retry_timer_callback);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

	wifi_config_t wifi_config = {
		.sta =
			{
				.threshold.authmode = WIFI_AUTH_WPA2_PSK,
				.pmf_cfg = {.capable = true, .required = false},
			},
	};

	char ssid[32] = {0};
	char pass[64] = {0};
	osj_nvs_get_str("apSsid", ssid, sizeof(ssid), "");
	osj_nvs_get_str("apPasswd", pass, sizeof(pass), "");

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	if (strlen(ssid) > 0) {
		strncpy((char *)wifi_config.sta.ssid, ssid,
				sizeof(wifi_config.sta.ssid));
		strncpy((char *)wifi_config.sta.password, pass,
				sizeof(wifi_config.sta.password));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	} else {
		ESP_LOGW(TAG, "No SSID in NVS, starting AP mode for provisioning.");
        start_softap();
	}

	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "WiFi init finished.");
}

bool osj_wifi_is_connected(void) { return s_is_connected; }

void osj_wifi_get_mac(char *mac_str) {
	uint8_t mac[6];
	esp_wifi_get_mac(WIFI_IF_STA, mac);
	sprintf(mac_str, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3],
			mac[4], mac[5]);
}

void osj_wifi_get_ip(char *ip_str) {
	esp_netif_ip_info_t ip_info;
	esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
	if (netif) {
		esp_netif_get_ip_info(netif, &ip_info);
		sprintf(ip_str, IPSTR, IP2STR(&ip_info.ip));
	} else {
		strcpy(ip_str, "0.0.0.0");
	}
}

int8_t osj_wifi_get_rssi(void) {
	wifi_ap_record_t ap_info;
	if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
		return ap_info.rssi;
	}
	return -127;
}