#ifndef OSJ_NVS_H
#define OSJ_NVS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief NVS 파티션을 초기화한다.
 */
void osj_nvs_init(void);

/**
 * @brief 문자열 값을 NVS에 저장한다.
 * @param key 저장할 키 이름
 * @param value 저장할 문자열 값
 */
void osj_nvs_set_str(const char *key, const char *value);

/**
 * @brief NVS에서 문자열 값을 읽어온다.
 * @param key 읽어올 키 이름
 * @param out_value 값을 저장할 버퍼
 * @param max_len 버퍼의 최대 길이
 * @param default_value 값이 없을 경우 반환할 기본값
 */
void osj_nvs_get_str(const char *key, char *out_value, size_t max_len,
					 const char *default_value);

/**
 * @brief 실수형 값을 NVS에 저장한다.
 * @param key 저장할 키 이름
 * @param value 저장할 실수형 값
 */
void osj_nvs_set_float(const char *key, float value);

/**
 * @brief NVS에서 실수형 값을 읽어온다.
 * @param key 읽어올 키 이름
 * @param default_value 값이 없을 경우 반환할 기본값
 * @return 읽어온 실수형 값 또는 기본값
 */
float osj_nvs_get_float(const char *key, float default_value);

/**
 * @brief 부호 없는 정수형 값을 NVS에 저장한다.
 * @param key 저장할 키 이름
 * @param value 저장할 정수형 값
 */
void osj_nvs_set_uint(const char *key, uint32_t value);

/**
 * @brief NVS에서 부호 없는 정수형 값을 읽어온다.
 * @param key 읽어올 키 이름
 * @param default_value 값이 없을 경우 반환할 기본값
 * @return 읽어온 정수형 값 또는 기본값
 */
uint32_t osj_nvs_get_uint(const char *key, uint32_t default_value);

/**
 * @brief 논리형 값을 NVS에 저장한다.
 * @param key 저장할 키 이름
 * @param value 저장할 논리형 값
 */
void osj_nvs_set_bool(const char *key, bool value);

/**
 * @brief NVS에서 논리형 값을 읽어온다.
 * @param key 읽어올 키 이름
 * @param default_value 값이 없을 경우 반환할 기본값
 * @return 읽어온 논리형 값 또는 기본값
 */
bool osj_nvs_get_bool(const char *key, bool default_value);

#endif