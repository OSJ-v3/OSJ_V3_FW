#ifndef OSJ_CONFIG_H
#define OSJ_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char apSsid[32];
    char apPasswd[64];
    char roomNo[16];
    char authId[32];
    char authPasswd[32];
    
    char ch1DeviceNo[8];
    char ch2DeviceNo[8];
    
    float ch1CurrW;
    float ch2CurrW;
    uint32_t ch1FlowW;
    uint32_t ch2FlowW;
    float ch1CurrD;
    float ch2CurrD;
    
    uint32_t ch1EndDelayW;
    uint32_t ch2EndDelayW;
    uint32_t ch1EndDelayD;
    uint32_t ch2EndDelayD;
    
    bool isCh1Live;
    bool isCh2Live;
    
    float hysteresisMargin;
} SystemConfig;

extern SystemConfig sys_config;

void osj_config_lock(void);
void osj_config_unlock(void);

#endif
