
/** \file   acc_lis2_de.c
*   \author Alex
*   \date   26.09.2023
*   
* Реализация работы с акселерометром lis2de12
*/

#include "acc.h"

#include "lis2dh.h"

static const struct device *acc_lis2 = NULL;

struct lis2dh_data *lis2dh;

/** Прерывание по превышению уровня 
*/
void lis12dw_trigger_handler(const struct device *dev, const struct sensor_trigger *trig) {
  if (trig->type == SENSOR_TRIG_THRESHOLD) {
    uint8_t event = evIrqHi;
    send_event( event );
  }
  else if (trig->type == SENSOR_TRIG_FREEFALL) {
    uint8_t event = evIrqLo;
    send_event( event );
  }
  else if (trig->type == SENSOR_TRIG_DELTA) {
    uint8_t event = evIrqHi;
    send_event( event );
  }
}

void lis12dw_trigger_freefall_handler(const struct device *dev, const struct sensor_trigger *trig) {
  uint8_t event = evIrqLo;
  send_event( event );
}

int init_acc( void ) {
  int err;
  
  acc_lis2 = DEVICE_DT_GET_ANY(st_lis2dh);
  if (acc_lis2 == NULL) {
    return -1;
  }

  uint8_t reg_val = 0;
  struct lis2dh_data *lis2dh = acc_lis2->data;

  err = lis2dh->hw_tf->read_reg(acc_lis2, LIS2DH_REG_WAI, &reg_val);
  if (err < 0) {
    return -1;
    //    SELF_TEST_MESS("ACC", "ERROR");
    //    while (1) {
    //      k_sleep(K_SECONDS(1));
    //    }
  }

  if (reg_val != LIS2DH_CHIP_ID) {
    return -1;
    //    LOG_ERR("Invalid chip ID: %02x\n", reg_val);
    //    SELF_TEST_MESS("ACC", "ERROR");
    //    while (1) {
    //      k_sleep(K_SECONDS(1));
    //    }
  }

  struct sensor_value sval = {0};

  sval.val1 = ODR_VALUE;
  sval.val2 = 0;
  sensor_attr_set(acc_lis2, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sval);

  /* Установка шкалы */
  sval.val1 = ACC_FULL_SCALE;
  sval.val2 = 0;
  err = sensor_attr_set(acc_lis2, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &sval);
  if (err) {
//    SELF_TEST_MESS("ACC SCALE", "ERROR");
  }  
  
  return 0;
}

int read_adv_acc_data(adv_data_s *pdata) {
  int res = 0;
  res += lis2dh->hw_tf->read_reg(acc_lis2, 0x29, &pdata->x);
  res += lis2dh->hw_tf->read_reg(acc_lis2, 0x2b, &pdata->y);
  res += lis2dh->hw_tf->read_reg(acc_lis2, 0x2d, &pdata->z);
  return res;
}

int read_sensor_value( struct sensor_value* val ) {
  int res = 0;
  res += sensor_sample_fetch(acc_lis2);
  res += sensor_channel_get(acc_lis2, SENSOR_CHAN_ACCEL_XYZ, val);
  return res;
}