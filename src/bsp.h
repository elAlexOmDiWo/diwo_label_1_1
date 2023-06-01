
/** \file   bsp.h
*   \author Alex
*   \date   17.11.2022
*   
* �������� ����������� �����
*/

#pragma once

#include <stdint.h>

void init_board( void );

/** \brief ������ ����������� �����������
*   \return - ����������� � � ��� -127 � ������ ������
*/
int8_t get_temp( void );

/** \brief ������ ��������� �������
*   \return - ������ �������� � % ( 0% - 100% ), ��� -1 � ������ ������
*/
int8_t get_batt( int temp );