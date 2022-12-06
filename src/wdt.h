
/** \file		wdt.h
*		\author	Alex
*		\date		27.11.2022
*
*	���������� WDT ��� ����������
*/

#pragma once

#include <stdint.h>
#include <stdbool.h>

bool init_wdt( uint32_t period );

void reset_wdt( void );
