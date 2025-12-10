#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#include <stdbool.h>
#include <stdint.h>

#define DEVICE_ID_CH_1 101
#define DEVICE_ID_CH_2 102

typedef enum { MODE_WASHER = 0, MODE_DRYER = 1 } device_mode_t;

typedef enum {
	STATUS_IDLE = 0,
	STATUS_WORKING = 1,
	STATUS_ERROR = 2
} device_status_t;

#endif // COMMON_DEFS_H
