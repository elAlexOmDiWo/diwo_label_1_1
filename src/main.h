#pragma once

#include "diwo_label.h"

#include <stdbool.h>

#define __DEBUG__ 1
#define __ENABLE_SELF_TEST_MESS__ 1

#if (__DEBUG__ != 1)
#define ACC_SHOCK_THRESHOLD 60 // мС^2
#define ACC_SHOCK_DURATION 3

#define ACC_FREEFALL_THRESHOLD 7 //ff_thrs_7           // Порог события free-fall ( 0 - 7 )
#define ACC_FREFALL_DURATION 2

#define BLE_TX_POWER 0

#define __ENABLE_WDT__ 1
#define __SEGGER_FORMAT 1
#define ODR_VALUE 25
#define DEFAULT_ADV_PERIOD_S 3 // Период отсылки рекламы по умолчанию
#define ACC_FULL_SCALE 20      // мС^2
#else

#warning Debug. Change before release

#define ACC_SHOCK_THRESHOLD 40 // мС^2
#define ACC_SHOCK_DURATION 3

#define ACC_FREEFALL_THRESHOLD 2 //ff_thrs_7           // Порог события free-fall ( 0 - 7 )
#define ACC_FREFALL_DURATION 2

#define BLE_TX_POWER 0

#define __ENABLE_LED__  1
#define __ENABLE_BUTTON__ 1
#define __ENABLE_ACC__  1
#define __ENABLE_BLE__ 1
#define __ENABLE_WDT__ 1
#define __ENABLE_NFC__  1
#define __ENABLE_DEEP_SLEEP__ 1
#define __SEGGER_FORMAT 0
#define ODR_VALUE 100
#define DEFAULT_ADV_PERIOD_S 1 // Период отсылки рекламы по умолчанию
#define ACC_FULL_SCALE 20      // мС^2
#endif

#define LOG_ADV_DBG_ENABLE      0

#if( LOG_ADV_DBG_ENABLE != 0 )
#define LOG_ADV_DBG           LOG_DBG
#else#define LOG_ADV_DBG( ... )#endif

#if ( __DEBUG__ != 0 )
#define DEFAULT_PASSWORD   { 1, 2, 3, 4, 5, 6, 7, 8 }
#else
#define DEFAULT_PASSWORD   { 28,32,52,95,16,81,31,30 }
#endif

#ifndef APP_PASSWORD
#warning Use default password
#define APP_PASSWORD      DEFAULT_PASSWORD  
#endif

#if (__DEBUG__ != 0)
#define WAITE_CONNECTION_TIME 10
#define WAITE_DISCONNECTION_TIME 1200
#else
#define WAITE_CONNECTION_TIME 10
#define WAITE_DISCONNECTION_TIME 120
#endif

#ifndef ADV_PERIOD_MIN
#define ADV_PERIOD_MIN       1
#endif
#ifndef ADV_PERIOD_MAX
#define ADV_PERIOD_MAX       5
#endif

#ifndef POWER_MIN
#define POWER_MIN             -40
#endif

#ifndef POWER_MAX
#define POWER_MAX             4
#endif

#ifndef HIT_THRESHOLD_MIN
#define HIT_THRESHOLD_MIN     0
#endif

#ifndef HIT_THRESHOLD_MAX
#define HIT_THRESHOLD_MAX     16
#endif

#ifndef FALL_THRESHOLD_MIN
#define FALL_THRESHOLD_MIN    0
#endif

#ifndef FALL_THRESHOLD_MAX
#define FALL_THRESHOLD_MAX    16
#endif

#define KEY_EVENT_MIN ( 32768 * 0.5 )
#define KEY_EVENT_MAX ( 32768 * 1.5 )

typedef struct {
  int8_t adv_period;
  int8_t adv_period_min;  
  int8_t adv_period_max;

  int8_t power;
  int8_t power_min;
  int8_t power_max;
  
  int8_t hit_threshold;
  int8_t hit_threshold_min;
  int8_t hit_threshold_max;
  
  int8_t fall_threshold;
  int8_t fall_threshold_min;
  int8_t fall_threshold_max;  

  uint8_t pass[8];
  bool open;
  bool acc_status;
} app_settings_s;

/** События приложения */
typedef enum {
  evNone = 0,
  evTimer,
  evIrqHi,
  evIrqLo,
  evButton,
  evButtonOnEnd,
  evCmdPowerOff,
} events_e;

typedef enum {
//  scmNone = 0,
//  scmPeriodSet = 1,
//  scmPowerSet = 2,
//  scmHitThreshold = 4,
//  scmFallThreshold = 8,  
  scmPowerOff = 0x95,
} settings_cmd_e;

extern app_settings_s app_settings;

int send_event(events_e event);