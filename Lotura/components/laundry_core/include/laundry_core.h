#ifndef LAUNDRY_CORE_H
#define LAUNDRY_CORE_H

/**
 * @brief 세탁/건조 로직을 수행하는 메인 태스크.
 * @param pvParameters 태스크 파라미터 (사용 안함)
 */
void laundry_core_task(void *pvParameters);

/**
 * @brief 현재 세탁/건조 상태를 JSON 문자열로 반환한다.
 * @return JSON 문자열 (호출자가 free해야 함)
 */
char *laundry_core_get_status_json(void);

#endif