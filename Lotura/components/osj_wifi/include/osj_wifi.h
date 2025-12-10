#ifndef OSJ_WIFI_H
#define OSJ_WIFI_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief WiFi를 초기화하고 연결을 시도한다.
 */
void osj_wifi_init(void);

/**
 * @brief WiFi 연결 상태를 반환한다.
 * @return 연결되었으면 true, 아니면 false
 */
bool osj_wifi_is_connected(void);

/**
 * @brief 디바이스의 MAC 주소를 문자열로 반환한다.
 * @param mac_str MAC 주소를 저장할 버퍼 (최소 13바이트)
 */
void osj_wifi_get_mac(char *mac_str);

/**
 * @brief 디바이스의 IP 주소를 문자열로 반환한다.
 * @param ip_str IP 주소를 저장할 버퍼 (최소 16바이트)
 */
void osj_wifi_get_ip(char *ip_str);

/**
 * @brief 현재 연결된 AP의 신호 강도(RSSI)를 반환한다.
 * @return RSSI 값 (dBm)
 */
int8_t osj_wifi_get_rssi(void);

#endif