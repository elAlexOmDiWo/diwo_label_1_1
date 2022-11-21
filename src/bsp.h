
/** \file   bsp.h
*   \author Alex
*   \date   17.11.2022
*   
* Описание функционала платы
*/

#pragma once

#include <stdint.h>

void init_board( void );

/** \brief Чтение температуры контроллера
*   \return - температура в С или -127 в случае ошибки
*/
int8_t get_temp( void );

/** \brief Чтение состояния батареи
*   \return - статус батареии в % ( 0% - 100% ), или -1 в случае ошибки
*/
int8_t get_batt( int temp );