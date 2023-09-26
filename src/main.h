#pragma once

#include "diwo_label.h"

#define __DEBUG__ 1
#define __ENABLE_SELF_TEST_MESS__ 1

#if (__DEBUG__ != 1)
#define ACC_SHOCK_THRESHOLD 60 // м—^2
#define __ENABLE_WDT__ 1
#define __SEGGER_FORMAT 1
#define ODR_VALUE 25
#define DEFAULT_ADV_PERIOD_S 3 // ѕериод отсылки рекламы по умолчанию
#define ACC_FULL_SCALE 20      // м—^2
#else

#warning Debug. Change before release

#define ACC_SHOCK_THRESHOLD 40 // м—^2
#define ACC_SHOCK_DURATION 3

#define __ENABLE_LED__  1
#define __ENABLE_BUTTON__ 1
#define __ENABLE_ACC__  0
#define __ENABLE_BLE__ 0
#define __ENABLE_WDT__ 0
#define __ENABLE_NFC__  0
#define __ENABLE_DEEP_SLEEP__ 0
#define __SEGGER_FORMAT 1
#define ODR_VALUE 100
#define DEFAULT_ADV_PERIOD_S 1 // ѕериод отсылки рекламы по умолчанию
#define ACC_FULL_SCALE 20      // м—^2
#endif

typedef struct {
  int adv_period;
  int power;
  int hit_threshold;
  int fall_threshold;
} app_settings_s;

/** —обыти€ приложени€ */
typedef enum {
  evNone = 0,
  evTimer,
  evIrqHi,
  evIrqLo,
  evButton,
} events_e;

extern app_settings_s app_settings;

int send_event(events_e event);