#include "osj_sensor.h"
#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio_definitions.h"
#include "osj_gpio.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static volatile uint32_t flow_freq_1 = 0;
static volatile uint32_t flow_freq_2 = 0;

static adc_oneshot_unit_handle_t adc1_handle;

static void IRAM_ATTR flow_sensor_isr_handler_1(void *arg) { flow_freq_1++; }
static void IRAM_ATTR flow_sensor_isr_handler_2(void *arg) { flow_freq_2++; }

void osj_sensor_init(void) {
	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_NEGEDGE;
	io_conf.pin_bit_mask =
		(1ULL << PIN_FLOW_SENSOR_1) | (1ULL << PIN_FLOW_SENSOR_2);
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = 0;
	io_conf.pull_down_en = 0;
	gpio_config(&io_conf);

	gpio_install_isr_service(0);
	gpio_isr_handler_add(PIN_FLOW_SENSOR_1, flow_sensor_isr_handler_1, NULL);
	gpio_isr_handler_add(PIN_FLOW_SENSOR_2, flow_sensor_isr_handler_2, NULL);

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
	int samples = 1480;
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
	if (channel == 1)
		flow_freq_1 = 0;
	else
		flow_freq_2 = 0;
}

int osj_sensor_get_drain(int channel) {
	if (channel == 1)
		return FAST_GPIO_READ(PIN_DRAIN_SENSOR_1);
	else
		return FAST_GPIO_READ(PIN_DRAIN_SENSOR_2);
}