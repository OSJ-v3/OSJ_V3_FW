#ifndef OSJ_WEBSOCKET_H
#define OSJ_WEBSOCKET_H

/**
 * @brief 웹소켓 클라이언트를 시작한다.
 */
void osj_websocket_start(void);

/**
 * @brief 특정 채널의 상태를 서버로 전송한다.
 * @param channel 채널 번호 (1 또는 2)
 * @param status 상태 값 (0: 동작 중, 1: 대기 중)
 */
void osj_websocket_send_status(int channel, int status);

/**
 * @brief 특정 채널의 로그 데이터를 서버로 전송한다.
 * @param channel 채널 번호 (1 또는 2)
 * @param log_json JSON 형식의 로그 문자열
 */
void osj_websocket_send_log(int channel, const char *log_json);

#endif