#pragma once

#include "diwo_label.h"

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
#define __ENABLE_BLE__ 0
#define __ENABLE_WDT__ 0
#define __ENABLE_NFC__  0
#define __ENABLE_DEEP_SLEEP__ 0
#define __SEGGER_FORMAT 0
#define ODR_VALUE 100
#define DEFAULT_ADV_PERIOD_S 1 // Период отсылки рекламы по умолчанию
#define ACC_FULL_SCALE 20      // мС^2
#endif

typedef struct {
  int adv_period;
  int power;
  int hit_threshold;
  int fall_threshold;
} app_settings_s;

/** События приложения */
typedef enum {
  evNone = 0,
  evTimer,
  evIrqHi,
  evIrqLo,
  evButton,
} events_e;

typedef enum {
  srNone = 0,
  srPowerOff,
} settings_result_e;

extern app_settings_s app_settings;

int send_event(events_e event);