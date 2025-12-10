/**
 * @file gpio_definitions.h
 * @brief ESP32를 위한 고속 GPIO 제어 정의.
 * @details 이 헤더는 표준 ESP-IDF 드라이버 오버헤드를 우회하여 초고속 GPIO
 * 조작을 위한 직접 메모리 액세스(MMIO) 매크로를 제공한다. 원자적 비트 연산과
 * 병렬 포트 쓰기를 지원한다.
 */
#include <stdint.h>

#ifndef GPIO_DEFINITIONS_H
#define GPIO_DEFINITIONS_H

/**
 * @name 메모리 맵 기본 주소
 * @brief GPIO 및 IO MUX 주변기기의 기본 주소 (참조: ESP32 TRM).
 * @{
 */
#ifndef DR_REG_GPIO_BASE
#define DR_REG_GPIO_BASE 0x3FF44000 ///< GPIO 컨트롤러 기본 주소
#endif
#ifndef DR_REG_IO_MUX_BASE
#define DR_REG_IO_MUX_BASE 0x3FF49000 ///< IO MUX 컨트롤러 기본 주소
#endif
/** @} */

/**
 * @name 핀 정의
 * @brief 하드웨어 회로도에 기반한 GPIO 핀 할당.
 * @{
 */

/* 출력 핀 (뱅크 0: GPIO 0~31) */
#define PIN_STATUS_LED 17 ///< 시스템 상태 표시 LED
#define PIN_CH1_LED 18	  ///< 채널 1 상태 LED
#define PIN_CH2_LED 19	  ///< 채널 2 상태 LED

/* 입력 핀 (뱅크 0: GPIO 0~31) */
#define PIN_DEBUG_MODE 14	  ///< 디버그 모드 스위치 (Active Low)
#define PIN_DRAIN_SENSOR_1 23 ///< CH1 배수/진동 센서
#define PIN_DRAIN_SENSOR_2 25 ///< CH2 배수/진동 센서
#define PIN_FLOW_SENSOR_1 27  ///< CH1 유량 센서 (펄스)
#define PIN_FLOW_SENSOR_2 26  ///< CH2 유량 센서 (펄스)

/* 입력 핀 (뱅크 1: GPIO 32~39) - 입력 전용 */
#define PIN_CH2_MODE 32	   ///< CH2 모드 선택기 (세탁/건조)
#define PIN_CH1_MODE 33	   ///< CH1 모드 선택기 (세탁/건조)
#define PIN_CT_SENSOR_2 34 ///< CH2 전류 센서 (ADC1_CH6)
#define PIN_CT_SENSOR_1 35 ///< CH1 전류 센서 (ADC1_CH7)
/** @} */

/**
 * @name 레지스터 정의 (MMIO)
 * @brief 하드웨어 레지스터에 대한 직접 액세스 포인터.
 * @note 이 포인터에 액세스하면 하드웨어에 대한 직접 로드/스토어 명령어가
 * 호출된다.
 * @{
 */

/* ---------------- 뱅크 0 (GPIO 0 ~ 31) ---------------- */

/**
 * @brief GPIO 0-31 출력 레지스터.
 * @details 이 레지스터에 쓰면 32개 핀 모두에 동시에 영향을 준다.
 * 병렬 포트 에뮬레이션에 유용하다.
 */
#define GPIO_OUT_REG (*(volatile uint32_t *)(DR_REG_GPIO_BASE + 0x0004))

/**
 * @brief GPIO 0-31 출력 설정 레지스터 (W1TS).
 * @details 비트에 1을 쓰면 해당 핀을 High로 설정한다. 0을 쓰면 아무 효과가
 * 없다. 원자적 연산이다.
 */
#define GPIO_OUT_W1TS_REG (*(volatile uint32_t *)(DR_REG_GPIO_BASE + 0x0008))

/**
 * @brief GPIO 0-31 출력 클리어 레지스터 (W1TC).
 * @details 비트에 1을 쓰면 해당 핀을 Low로 설정한다. 0을 쓰면 아무 효과가 없다.
 * 원자적 연산이다.
 */
#define GPIO_OUT_W1TC_REG (*(volatile uint32_t *)(DR_REG_GPIO_BASE + 0x000C))

/**
 * @brief GPIO 0-31 출력 활성화 레지스터.
 * @details 비트를 1로 설정하면 출력, 0이면 입력(High-Z)이다.
 */
#define GPIO_ENABLE_REG (*(volatile uint32_t *)(DR_REG_GPIO_BASE + 0x0020))

/** @brief GPIO 0-31 출력 활성화 설정 레지스터 (W1TS). */
#define GPIO_ENABLE_W1TS_REG (*(volatile uint32_t *)(DR_REG_GPIO_BASE + 0x0024))

/** @brief GPIO 0-31 출력 활성화 클리어 레지스터 (W1TC). */
#define GPIO_ENABLE_W1TC_REG (*(volatile uint32_t *)(DR_REG_GPIO_BASE + 0x0028))

/** @brief GPIO 0-31 입력 값 레지스터 (읽기 전용). */
#define GPIO_IN_REG (*(volatile uint32_t *)(DR_REG_GPIO_BASE + 0x003C))

/* ---------------- 뱅크 1 (GPIO 32 ~ 39) ---------------- */

/** @brief GPIO 32-39 출력 레지스터. */
#define GPIO_OUT1_REG (*(volatile uint32_t *)(DR_REG_GPIO_BASE + 0x0010))

/** @brief GPIO 32-39 출력 설정 레지스터 (W1TS). */
#define GPIO_OUT1_W1TS_REG (*(volatile uint32_t *)(DR_REG_GPIO_BASE + 0x0014))

/** @brief GPIO 32-39 출력 클리어 레지스터 (W1TC). */
#define GPIO_OUT1_W1TC_REG (*(volatile uint32_t *)(DR_REG_GPIO_BASE + 0x0018))

/** @brief GPIO 32-39 출력 활성화 레지스터. */
#define GPIO_ENABLE1_REG (*(volatile uint32_t *)(DR_REG_GPIO_BASE + 0x002C))

/** @brief GPIO 32-39 출력 활성화 설정 레지스터 (W1TS). */
#define GPIO_ENABLE1_W1TS_REG                                                  \
	(*(volatile uint32_t *)(DR_REG_GPIO_BASE + 0x0030))

/** @brief GPIO 32-39 출력 활성화 클리어 레지스터 (W1TC). */
#define GPIO_ENABLE1_W1TC_REG                                                  \
	(*(volatile uint32_t *)(DR_REG_GPIO_BASE + 0x0034))

/** @brief GPIO 32-39 입력 값 레지스터 (읽기 전용). */
#define GPIO_IN1_REG (*(volatile uint32_t *)(DR_REG_GPIO_BASE + 0x0040))
/** @} */

/**
 * @name 초고속 매크로
 * @brief 최적화된 GPIO 제어를 위한 헬퍼 매크로.
 * @{
 */

/**
 * @brief GPIO 핀을 원자적으로 High로 설정한다.
 * @param pin GPIO 번호 (0-39).
 * @note 원자적 액세스를 위해 W1TS 레지스터를 사용한다.
 */
#define FAST_GPIO_SET(pin)                                                     \
	((pin < 32) ? (GPIO_OUT_W1TS_REG = (1UL << (pin)))                         \
				: (GPIO_OUT1_W1TS_REG = (1UL << ((pin) - 32))))

/**
 * @brief GPIO 핀을 원자적으로 Low로 설정한다.
 * @param pin GPIO 번호 (0-39).
 * @note 원자적 액세스를 위해 W1TC 레지스터를 사용한다.
 */
#define FAST_GPIO_CLEAR(pin)                                                   \
	((pin < 32) ? (GPIO_OUT_W1TC_REG = (1UL << (pin)))                         \
				: (GPIO_OUT1_W1TC_REG = (1UL << ((pin) - 32))))

/**
 * @brief 전체 GPIO 0-31 포트에 값을 동시에 쓴다.
 * @param val GPIO_OUT_REG에 쓸 32비트 값.
 * @warning 이것은 뱅크 0 (GPIO 0-31)의 모든 핀 상태를 덮어쓴다.
 */
#define FAST_GPIO_WRITE_ALL(val) (GPIO_OUT_REG = (val))

/**
 * @brief GPIO 핀의 현재 입력 값을 읽는다.
 * @param pin GPIO 번호 (0-39).
 * @return High면 1, Low면 0.
 */
#define FAST_GPIO_READ(pin)                                                    \
	((pin < 32) ? ((GPIO_IN_REG >> (pin)) & 1UL)                               \
				: ((GPIO_IN1_REG >> ((pin) - 32)) & 1UL))

/**
 * @brief 핀을 출력으로 설정한다.
 * @param pin GPIO 번호 (0-39).
 */
#define FAST_GPIO_OUTPUT_EN(pin)                                               \
	((pin < 32) ? (GPIO_ENABLE_W1TS_REG = (1UL << (pin)))                      \
				: (GPIO_ENABLE1_W1TS_REG = (1UL << ((pin) - 32))))

/**
 * @brief 핀을 입력으로 설정한다 (출력 비활성화).
 * @param pin GPIO 번호 (0-39).
 */
#define FAST_GPIO_INPUT_EN(pin)                                                \
	((pin < 32) ? (GPIO_ENABLE_W1TC_REG = (1UL << (pin)))                      \
				: (GPIO_ENABLE1_W1TC_REG = (1UL << ((pin) - 32))))
/** @} */

#endif // GPIO_DEFINITIONS_H
