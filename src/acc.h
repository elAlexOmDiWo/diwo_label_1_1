#pragma once

#include <stdint.h>

#include "main.h"

#include <zephyr/drivers/sensor.h>

typedef struct {
  uint32_t odr;
} acc_setting_t;

typedef struct {
  int x;
  int y;
  int z;
} acc_value_xyz_s;

int init_acc(void);

int read_adv_acc_data(adv_data_s *pdata);

int read_sensor_value(struct sensor_value *val);