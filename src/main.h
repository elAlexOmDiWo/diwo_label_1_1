#pragma once

#include "diwo_label.h"

#include <stdbool.h>

#define __DEBUG__ 1
#define __ENABLE_SELF_TEST_MESS__ 1

#if (__DEBUG__ != 1)
#define ACC_SHOCK_THRESHOLD 60 // мС^2
#define __ENABLE_WDT__ 1
#define __SEGGER_FORMAT 1
#define ODR_VALUE 25
#define DEFAULT_ADV_PERIOD_S 3 // Период отсылки рекламы по умолчанию
#define ACC_FULL_SCALE 20      // мС^2
#else

#warning Debug. Change before release

#define ACC_SHOCK_THRESHOLD 40 // мС^2
#define ACC_SHOCK_DURATION 3

#define __ENABLE_LED__  1
#define __ENABLE_BUTTON__ 1
#define __ENABLE_ACC__  0
#define __ENABLE_BLE__ 1
#define __ENABLE_WDT__ 0
#define __ENABLE_NFC__  0
#define __ENABLE_DEEP_SLEEP__ 1
#define __SEGGER_FORMAT 0
#define ODR_VALUE 100
#define DEFAULT_ADV_PERIOD_S 1 // Период отсылки рекламы по умолчанию
#define ACC_FULL_SCALE 20      // мС^2
#endif

#if ( __DEBUG__ != 0 )
#define DEFAULT_PASSWORD   { 1, 2, 3, 4, 5, 6, 7, 8 }
#else
#define DEFAULT_PASSWORD   { 2,3,5,9,1,8,3,3 }
#endif

#ifndef APP_PASSWORD
#warning Use default password
#define APP_PASSWORD      DEFAULT_PASSWORD  
#endif

#ifndef ADV_PERIOD_MIN
#define ADV_PERIOD_MIN       1
#endif
#ifndef ADV_PERIOD_MAX
#define ADV_PERIOD_MAX       3
#endif

#ifndef POWER_MIN
#define POWER_MIN             -40
#endif

#ifndef POWER_MAX
#define POWER_MAX             4
#endif

#ifndef HIT_THRESHOLD_MIN
#define HIT_THRESHOLD_MIN     
#endif

#ifndef HIT_THRESHOLD_MAX
#define HIT_THRESHOLD_MAX   
#endif

#ifndef FALL_THRESHOLD_MIN
#define FALL_THRESHOLD_MIN
#endif

#ifndef FALL_THRESHOLD_MAX
#define FALL_THRESHOLD_MAX
#endif

typedef struct {
  int adv_period;
  int adv_period_min;  
  int adv_period_max;

  int power;
  int power_min;
  int power_max;
  int hit_threshold;
  int hit_threshold_min;
  int hit_threshold_max;
 
  int fall_threshold;
  int fall_threshold_min;
  int fall_threshold_max;  

  uint8_t pass[8];
  bool open;
} app_settings_s;

/** События приложения */
typedef enum {
  evNone = 0,
  evTimer,
  evIrqHi,
  evIrqLo,
  evButton,
  evCmdPowerOff,
} events_e;

typedef enum {
  scmNone = 0,
  scmPowerOff = 0x95,
} settings_cmd_e;

extern app_settings_s app_settings;

int send_event(events_e event);