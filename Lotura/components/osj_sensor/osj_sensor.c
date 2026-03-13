#include "osj_sensor.h"
#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/pulse_cnt.h"
#include "gpio_definitions.h"
#include "osj_gpio.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static volatile uint32_t flow_freq_1 = 0;
static volatile uint32_t flow_freq_2 = 0;

static pcnt_unit_handle_t pcnt_unit_1 = NULL;
static pcnt_unit_handle_t pcnt_unit_2 = NULL;
static int16_t last_count_1 = 0;
static int16_t last_count_2 = 0;

static void pcnt_poll_timer_cb(TimerHandle_t xTimer) {
    if (!pcnt_unit_1 || !pcnt_unit_2) return;
    
    int count1 = 0, count2 = 0;
    pcnt_unit_get_count(pcnt_unit_1, &count1);
    pcnt_unit_get_count(pcnt_unit_2, &count2);

    int16_t c1 = (int16_t)count1;
    int16_t c2 = (int16_t)count2;

    flow_freq_1 += (uint16_t)(c1 - last_count_1);
    flow_freq_2 += (uint16_t)(c2 - last_count_2);

    last_count_1 = c1;
    last_count_2 = c2;
}

static adc_oneshot_unit_handle_t adc1_handle;

void osj_sensor_init(void) {
    pcnt_unit_config_t unit_config = {
        .high_limit = 32767,
        .low_limit = -32768,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit_1));
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit_2));

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    pcnt_unit_set_glitch_filter(pcnt_unit_1, &filter_config);
    pcnt_unit_set_glitch_filter(pcnt_unit_2, &filter_config);

    pcnt_chan_config_t chan_config_1 = {
        .edge_gpio_num = PIN_FLOW_SENSOR_1,
        .level_gpio_num = -1,
    };
    pcnt_channel_handle_t pcnt_chan_1 = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit_1, &chan_config_1, &pcnt_chan_1));

    pcnt_chan_config_t chan_config_2 = {
        .edge_gpio_num = PIN_FLOW_SENSOR_2,
        .level_gpio_num = -1,
    };
    pcnt_channel_handle_t pcnt_chan_2 = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit_2, &chan_config_2, &pcnt_chan_2));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_1, PCNT_CHANNEL_EDGE_ACTION_HOLD, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_2, PCNT_CHANNEL_EDGE_ACTION_HOLD, PCNT_CHANNEL_EDGE_ACTION_INCREASE));

    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit_1));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit_1));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit_1));

    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit_2));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit_2));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit_2));

    TimerHandle_t pcnt_timer = xTimerCreate("pcnt_poll", pdMS_TO_TICKS(1000), pdTRUE, NULL, pcnt_poll_timer_cb);
    xTimerStart(pcnt_timer, 0);

	adc_oneshot_unit_init_cfg_t init_config1 = {
		.unit_id = ADC_UNIT_1,
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

	adc_oneshot_chan_cfg_t config = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,
		.atten = ADC_ATTEN_DB_12,
	};
	ESP_ERROR_CHECK(
		adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &config));
	ESP_ERROR_CHECK(
		adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_7, &config));
}

float osj_sensor_get_rms(int channel) {
	int samples = 300;
	double sum_sq = 0;
	int val;
	adc_channel_t adc_channel = (channel == 1) ? ADC_CHANNEL_7 : ADC_CHANNEL_6;

	for (int i = 0; i < samples; i++) {
		ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adc_channel, &val));
		sum_sq += (val - 2048) * (val - 2048);
	}

	float irms = sqrt(sum_sq / samples);
	return irms;
}

uint32_t osj_sensor_get_flow(int channel) {
	if (channel == 1)
		return flow_freq_1;
	else
		return flow_freq_2;
}

void osj_sensor_reset_flow(int channel) {
    // Cumulative counter logic: do not reset.
	// if (channel == 1)
	// 	flow_freq_1 = 0;
	// else
	// 	flow_freq_2 = 0;
}

int osj_sensor_get_drain(int channel) {
	if (channel == 1)
		return FAST_GPIO_READ(PIN_DRAIN_SENSOR_1);
	else
		return FAST_GPIO_READ(PIN_DRAIN_SENSOR_2);
}