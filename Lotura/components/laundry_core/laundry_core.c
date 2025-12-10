#include "laundry_core.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio_definitions.h"
#include "osj_gpio.h"
#include "osj_nvs.h"
#include "osj_sensor.h"
#include "osj_websocket.h"
#include <math.h>
#include <string.h>

static const char *TAG = "LAUNDRY_CORE";

static float ch1CurrW = 0.2, ch2CurrW = 0.2;
static int ch1FlowW = 50, ch2FlowW = 50;
static float ch1CurrD = 0.5, ch2CurrD = 0.5;
static int ch1EndDelayW = 100000, ch2EndDelayW = 100000;
static int ch1EndDelayD = 10000, ch2EndDelayD = 10000;

static int ch1CurrStatus = 1;
static int ch2CurrStatus = 1;

static int64_t previousMillisEnd1 = 0;
static int64_t previousMillisEnd2 = 0;

static bool isCh1Mode = false;
static bool isCh2Mode = false;

static float ampsTrms1 = 0;
static float ampsTrms2 = 0;
static uint32_t lHour1 = 0;
static uint32_t lHour2 = 0;
static int waterSensorData1 = 0;
static int waterSensorData2 = 0;

static bool jsonLogFlag1 = false, jsonLogFlag2 = false;
static int jsonLogFlag1C = 0, jsonLogFlag2C = 0;
static int jsonLogFlag1F = 0, jsonLogFlag2F = 0;
static int jsonLogFlag1W = 0, jsonLogFlag2W = 0;
static int jsonLogCnt1 = 1, jsonLogCnt2 = 1;
static int64_t jsonLogMillis1 = 0, jsonLogMillis2 = 0;

static int ch1Cnt = 1, ch2Cnt = 1;
static int m1 = 0, m2 = 0;
static int seCnt1 = 0, seCnt2 = 0;
static int64_t sePrevMillis1 = 0, sePrevMillis2 = 0;

static int64_t millis() { return esp_timer_get_time() / 1000; }

static void send_log_entry(int channel, const char *type, int state,
						   int64_t start_time) {
	cJSON *log_obj = cJSON_CreateObject();
	char key[16];
	snprintf(key, sizeof(key), "%d",
			 (channel == 1) ? jsonLogCnt1 : jsonLogCnt2);

	cJSON *entry = cJSON_CreateObject();
	cJSON_AddNumberToObject(entry, "t", millis() - start_time);
	cJSON_AddStringToObject(entry, "n", type);
	cJSON_AddNumberToObject(entry, "s", state);

	cJSON_AddItemToObject(log_obj, key, entry);

	char *json_str = cJSON_PrintUnformatted(log_obj);
	if (json_str) {
		osj_websocket_send_log(channel, json_str);
		free(json_str);
	}
	cJSON_Delete(log_obj);
}

static void send_start_log(int channel) {
	cJSON *log_obj = cJSON_CreateObject();
	cJSON *entry = cJSON_CreateObject();
	cJSON_AddStringToObject(entry, "local_time", "");
	cJSON_AddItemToObject(log_obj, "START", entry);

	char *json_str = cJSON_PrintUnformatted(log_obj);
	if (json_str) {
		osj_websocket_send_log(channel, json_str);
		free(json_str);
	}
	cJSON_Delete(log_obj);
}

static void send_end_log(int channel) {
	cJSON *log_obj = cJSON_CreateObject();
	cJSON *entry = cJSON_CreateObject();
	cJSON_AddStringToObject(entry, "local_time", "");
	cJSON_AddItemToObject(log_obj, "END", entry);

	char *json_str = cJSON_PrintUnformatted(log_obj);
	if (json_str) {
		osj_websocket_send_log(channel, json_str);
		free(json_str);
	}
	cJSON_Delete(log_obj);
}

static void DryerStatusJudgment(float amps, int *cnt, int *m,
								int64_t *prevMillisEnd, int ch) {
	float currD = (ch == 1) ? ch1CurrD : ch2CurrD;
	int endDelay = (ch == 1) ? ch1EndDelayD : ch2EndDelayD;
	bool *logFlag = (ch == 1) ? &jsonLogFlag1 : &jsonLogFlag2;
	int *logFlagC = (ch == 1) ? &jsonLogFlag1C : &jsonLogFlag2C;
	int *logCnt = (ch == 1) ? &jsonLogCnt1 : &jsonLogCnt2;
	int64_t *logMillis = (ch == 1) ? &jsonLogMillis1 : &jsonLogMillis2;
	int *currStatus = (ch == 1) ? &ch1CurrStatus : &ch2CurrStatus;
	int ledPin = (ch == 1) ? PIN_CH1_LED : PIN_CH2_LED;

	if (amps < currD && *logFlag) {
		if (*logFlagC == 1) {
			*logFlagC = 0;
			send_log_entry(ch, "C", 0, *logMillis);
			(*logCnt)++;
		}
	}

	if (amps > currD) {
		if (*logFlag) {
			if (*logFlagC == 0) {
				*logFlagC = 1;
				send_log_entry(ch, "C", 1, *logMillis);
				(*logCnt)++;
			}
		}
		if (*cnt == 1) {
			*logFlag = true;
			*logCnt = 1;
			*logMillis = millis();
			send_start_log(ch);
			*cnt = 0;
			FAST_GPIO_SET(ledPin);
			*currStatus = 0;
			ESP_LOGI(TAG, "CH%d Dryer Started", ch);
			osj_websocket_send_status(ch, 0);
		}
		*m = 1;
	} else {
		if (*prevMillisEnd > millis())
			*prevMillisEnd = millis();

		if (*m) {
			*prevMillisEnd = millis();
			*m = 0;
		} else if (*cnt) {
		} else if ((millis() - *prevMillisEnd) >= endDelay) {
			*logFlagC = 0;
			*logFlag = false;
			send_end_log(ch);
			ESP_LOGI(TAG, "CH%d Dryer Ended", ch);
			osj_websocket_send_status(ch, 1);
			*cnt = 1;
			FAST_GPIO_CLEAR(ledPin);
			*currStatus = 1;
		}
	}
}

static void StatusJudgment(float amps, int water, uint32_t flow, int *cnt,
						   int *m, int64_t *prevMillisEnd, int ch) {
	float currW = (ch == 1) ? ch1CurrW : ch2CurrW;
	int flowW = (ch == 1) ? ch1FlowW : ch2FlowW;
	int endDelay = (ch == 1) ? ch1EndDelayW : ch2EndDelayW;

	bool *logFlag = (ch == 1) ? &jsonLogFlag1 : &jsonLogFlag2;
	int *logFlagC = (ch == 1) ? &jsonLogFlag1C : &jsonLogFlag2C;
	int *logFlagF = (ch == 1) ? &jsonLogFlag1F : &jsonLogFlag2F;
	int *logFlagW = (ch == 1) ? &jsonLogFlag1W : &jsonLogFlag2W;
	int *logCnt = (ch == 1) ? &jsonLogCnt1 : &jsonLogCnt2;
	int64_t *logMillis = (ch == 1) ? &jsonLogMillis1 : &jsonLogMillis2;

	int *seCnt = (ch == 1) ? &seCnt1 : &seCnt2;
	int64_t *sePrev = (ch == 1) ? &sePrevMillis1 : &sePrevMillis2;

	int *currStatus = (ch == 1) ? &ch1CurrStatus : &ch2CurrStatus;
	int ledPin = (ch == 1) ? PIN_CH1_LED : PIN_CH2_LED;

	if ((amps > currW || water || flow > flowW) && *seCnt == 0) {
		*seCnt = 1;
		*sePrev = millis();
	} else if ((amps < currW && !water && flow < flowW) && *seCnt == 1) {
		*seCnt = 0;
	}

	if (*logFlag) {
		if (amps > currW && *logFlagC == 0) {
			*logFlagC = 1;
			send_log_entry(ch, "C", 1, *logMillis);
			(*logCnt)++;
		} else if (amps < currW && *logFlagC == 1) {
			*logFlagC = 0;
			send_log_entry(ch, "C", 0, *logMillis);
			(*logCnt)++;
		}
		if (flow > flowW && *logFlagF == 0) {
			*logFlagF = 1;
			send_log_entry(ch, "F", 1, *logMillis);
			(*logCnt)++;
		} else if (flow < flowW && *logFlagF == 1) {
			*logFlagF = 0;
			send_log_entry(ch, "F", 0, *logMillis);
			(*logCnt)++;
		}
		if (water && *logFlagW == 0) {
			*logFlagW = 1;
			send_log_entry(ch, "W", 1, *logMillis);
			(*logCnt)++;
		} else if (!water && *logFlagW == 1) {
			*logFlagW = 0;
			send_log_entry(ch, "W", 0, *logMillis);
			(*logCnt)++;
		}
	}

	if ((millis() - *sePrev) >= 500 && *seCnt == 1) {
		if (*cnt == 1) {
			*logFlag = true;
			*logCnt = 1;
			*logMillis = millis();
			send_start_log(ch);
			*seCnt = 0;
			*cnt = 0;
			FAST_GPIO_SET(ledPin);
			*currStatus = 0;
			ESP_LOGI(TAG, "CH%d Washer Started", ch);
			osj_websocket_send_status(ch, 0);
		}
		*m = 1;
	} else {
		if (*prevMillisEnd > millis())
			*prevMillisEnd = millis();

		if (*m) {
			*prevMillisEnd = millis();
			*m = 0;
		} else if (*cnt) {
		} else if ((millis() - *prevMillisEnd) >= endDelay) {
			*logFlagC = 0;
			*logFlagF = 0;
			*logFlagW = 0;
			*logFlag = false;
			send_end_log(ch);
			ESP_LOGI(TAG, "CH%d Washer Ended", ch);
			osj_websocket_send_status(ch, 1);
			*cnt = 1;
			FAST_GPIO_CLEAR(ledPin);
			*currStatus = 1;
		}
	}
}

void laundry_core_task(void *pvParameters) {
	ESP_LOGI(TAG, "Laundry Core Task Started");

	ch1CurrW = osj_nvs_get_float("ch1CurrW", 0.2);
	ch2CurrW = osj_nvs_get_float("ch2CurrW", 0.2);
	ch1FlowW = osj_nvs_get_uint("ch1FlowW", 50);
	ch2FlowW = osj_nvs_get_uint("ch2FlowW", 50);
	ch1CurrD = osj_nvs_get_float("ch1CurrD", 0.5);
	ch2CurrD = osj_nvs_get_float("ch2CurrD", 0.5);
	ch1EndDelayW = osj_nvs_get_uint("ch1EndDelayW", 10) * 10000;

	ch1EndDelayW = osj_nvs_get_uint("ch1EndDelayW", 100000);
	ch2EndDelayW = osj_nvs_get_uint("ch2EndDelayW", 100000);
	ch1EndDelayD = osj_nvs_get_uint("ch1EndDelayD", 10000);
	ch2EndDelayD = osj_nvs_get_uint("ch2EndDelayD", 10000);

	while (1) {
		ampsTrms1 = osj_sensor_get_rms(1);
		ampsTrms2 = osj_sensor_get_rms(2);

		uint32_t freq1 = osj_sensor_get_flow(1);
		uint32_t freq2 = osj_sensor_get_flow(2);
		lHour1 = (freq1 * 60) / 7.5;
		lHour2 = (freq2 * 60) / 7.5;
		osj_sensor_reset_flow(1);
		osj_sensor_reset_flow(2);

		waterSensorData1 = osj_sensor_get_drain(1);
		waterSensorData2 = osj_sensor_get_drain(2);

		isCh1Mode = !FAST_GPIO_READ(PIN_CH1_MODE);
		isCh2Mode = !FAST_GPIO_READ(PIN_CH2_MODE);

		if (isCh1Mode) {
			StatusJudgment(ampsTrms1, waterSensorData1, lHour1, &ch1Cnt, &m1,
						   &previousMillisEnd1, 1);
		} else {
			DryerStatusJudgment(ampsTrms1, &ch1Cnt, &m1, &previousMillisEnd1,
								1);
		}

		if (isCh2Mode) {
			StatusJudgment(ampsTrms2, waterSensorData2, lHour2, &ch2Cnt, &m2,
						   &previousMillisEnd2, 2);
		} else {
			DryerStatusJudgment(ampsTrms2, &ch2Cnt, &m2, &previousMillisEnd2,
								2);
		}

		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

char *laundry_core_get_status_json(void) {
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "title", "GetData");
	cJSON_AddStringToObject(root, "ch1Status",
							ch1CurrStatus ? "Not Working" : "Working");
	cJSON_AddStringToObject(root, "ch2Status",
							ch2CurrStatus ? "Not Working" : "Working");
	cJSON_AddNumberToObject(root, "ch1Current", ampsTrms1);
	cJSON_AddNumberToObject(root, "ch2Current", ampsTrms2);

	char *json_str = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	return json_str;
}