
/** \file   power.h
*   \author Alex
*   \date   26.09.2023
*   
* Модуль управления питанием
*/

#pragma once

#include <hal/nrf_power.h>

uint32_t get_reset_reason(void);
