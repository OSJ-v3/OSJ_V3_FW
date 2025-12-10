#ifndef OSJ_SENSOR_H
#define OSJ_SENSOR_H

#include <stdint.h>

/**
 * @brief 센서를 초기화한다 (유량, ADC).
 */
void osj_sensor_init(void);

/**
 * @brief 특정 채널의 전류 RMS 값을 측정한다.
 * @param channel 채널 번호 (1 또는 2)
 * @return 측정된 전류 RMS 값 (A)
 */
float osj_sensor_get_rms(int channel);

/**
 * @brief 특정 채널의 유량 센서 펄스 수를 반환한다.
 * @param channel 채널 번호 (1 또는 2)
 * @return 누적된 유량 펄스 수
 */
uint32_t osj_sensor_get_flow(int channel);

/**
 * @brief 특정 채널의 유량 센서 펄스 수를 초기화한다.
 * @param channel 채널 번호 (1 또는 2)
 */
void osj_sensor_reset_flow(int channel);

/**
 * @brief 특정 채널의 배수 센서 상태를 읽는다.
 * @param channel 채널 번호 (1 또는 2)
 * @return 센서 상태 (1: 감지됨, 0: 감지안됨 - 센서 타입에 따라 다름)
 */
int osj_sensor_get_drain(int channel);

#endif