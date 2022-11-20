
/** \file   bsp.h
*   \author Alex
*   \date   17.11.2022
*   
* �������� ����������� �����
*/

#pragma once

#include <stdint.h>

void init_board( void );
int8_t get_batt_voltage( void );

/** \fn get_temp ������ ����������� �����������
* \return - ����������� � � ��� -127 � ������ ������
*/
int8_t get_temp( void );