
/** \file diwo_label.h
*   \author Alex
*   \date   14.11.2022
*   
* Константы спецефичные для метки diwo
*/

#pragma once

#include <stdint.h>

#define DIWO_ID                     0x3456
#define DIWO_LABEL_ACC_DATA_SIZE    8      

#pragma pack(push, 1)

/** Описание рекламного пакета */
typedef struct {
  uint16_t man_id;
  int8_t x;
  int8_t y;
  int8_t z;
  uint8_t bat;
  int8_t temp;
  uint8_t shock_value;
  uint8_t shock;
  uint8_t fall;
  uint8_t counter;
  uint8_t reserv[14];
} adv_data_s;

#pragma pack(pop)