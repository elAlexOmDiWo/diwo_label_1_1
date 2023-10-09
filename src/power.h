
/** \file   power.h
*   \author Alex
*   \date   26.09.2023
*   
* Модуль управления питанием
*/

#pragma once

#include <hal/nrf_power.h>

typedef enum {
  txp4p       = 0,
  txp3p       = 1,
  txp0        = 2,
  txp4m       = 3,
  txp8m       = 4,
  txp12m      = 5,
  txp16m      = 6,
  txp20m      = 7,
  txp40m      = 8,
} tx_power_e;

uint32_t get_reset_reason(void);

int set_power(int8_t tx_pwr_lvl);

int get_power_by_index(int index);

void set_power_tx(tx_power_e power);
