
/** \file   bsp.c
*   \author Alex
*   \date   17.11.2022
*   
* Описание функционала платы
*/

#include "bsp.h"

#include <zephyr/zephyr.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>

static const struct device *temp_dev = DEVICE_DT_GET_ANY( nordic_nrf_temp );

typedef struct {
  /** Remaining life at #lvl_mV. */
  uint16_t lvl_pptt;

  /** Battery voltage at #lvl_pptt remaining life. */
  uint16_t lvl_mV;
} battery_level_point;

typedef struct {
  uint8_t channel;
} io_channel_config;

struct divider_data {
  const struct device *adc;
  struct adc_channel_cfg adc_cfg;
  struct adc_sequence adc_seq;
  int16_t raw;
};

typedef struct {
  io_channel_config io_channel;
  struct gpio_dt_spec power_gpios;
  /* output_ohm is used as a flag value: if it is nonzero then
   * the battery is measured through a voltage divider;
   * otherwise it is assumed to be directly connected to Vdd.
   */
  uint32_t output_ohm;
  uint32_t full_ohm;
} divider_config_s;


//static const divider_config_s divider_config = {
//#if DT_NODE_HAS_STATUS(VBATT, okay)
//  .io_channel = {
//  DT_IO_CHANNELS_INPUT( VBATT ),
//},
//  .power_gpios = GPIO_DT_SPEC_GET_OR( VBATT, power_gpios, { } ),
//  .output_ohm = DT_PROP( VBATT, output_ohms ),
//  .full_ohm = DT_PROP( VBATT, full_ohms ),
//#else /* /vbatt exists */
//  .io_channel = {
//  DT_IO_CHANNELS_INPUT( ZEPHYR_USER ),
//},
//#endif /* /vbatt exists */
//};

static const battery_level_point levels[] = {
  /* "Curve" here eyeballed from captured data for the [Adafruit
   * 3.7v 2000 mAh](https://www.adafruit.com/product/2011) LIPO
   * under full load that started with a charge of 3.96 V and
   * dropped about linearly to 3.58 V over 15 hours.  It then
   * dropped rapidly to 3.10 V over one hour, at which point it
   * stopped transmitting.
   *
   * Based on eyeball comparisons we'll say that 15/16 of life
   * goes between 3.95 and 3.55 V, and 1/16 goes between 3.55 V
   * and 3.1 V.
   */

  { 10000, 3950 },
  { 625, 3550 },
  { 0, 3100 },
};

//static struct divider_data divider_data = {
//#if DT_NODE_HAS_STATUS(VBATT, okay)
//  .adc = DEVICE_DT_GET( DT_IO_CHANNELS_CTLR( VBATT ) ),
//#else
//  .adc = DEVICE_DT_GET( DT_IO_CHANNELS_CTLR( ZEPHYR_USER )),
//#endif
//};

//unsigned int battery_level_pptt( unsigned int batt_mV, const battery_level_point *curve );

void init_board ( void ) {
  if (temp_dev == NULL || !device_is_ready( temp_dev )) {
    printk( "no temperature device; using simulated data\n" );
    temp_dev = NULL;
  }  
}
//int8_t get_batt_voltage( void ) {
//  int rc = -ENOENT;
//
//  if ( true ) {
//    struct divider_data *ddp = &divider_data;
//    const divider_config_s *dcp = &divider_config;
//    struct adc_sequence *sp = &ddp->adc_seq;
//
//    rc = adc_read( ddp->adc, sp );
//    sp->calibrate = false;
//    if (rc == 0) {
//      int32_t val = ddp->raw;
//
//      adc_raw_to_millivolts(adc_ref_internal( ddp->adc ),
//        ddp->adc_cfg.gain,
//        sp->resolution,
//        &val  );
//
//      if (dcp->output_ohm != 0) {
//        rc = val * (uint64_t)dcp->full_ohm
//        	/ dcp->output_ohm;
//        printk("raw %u ~ %u mV => %d mV\n", ddp->raw, val, rc  );
//      }
//      else {
//        rc = val;
//        printk( "raw %u ~ %u mV\n", ddp->raw, val );
//      }
//    }
//  }
//  return rc;
//}

int8_t get_temp( void ) {
  int error = 0;
  struct sensor_value temp_value;
  
  if (temp_dev == NULL) return -127;
  
  error = sensor_sample_fetch( temp_dev );
  if (error) {
    return -127;
  }

  error = sensor_channel_get( temp_dev, SENSOR_CHAN_DIE_TEMP, &temp_value );
  if (error) {
    return -127;
  }
  return (int8_t)temp_value.val1;
}

//unsigned int battery_level_pptt(unsigned int batt_mV, const battery_level_point *curve  ) {
//  const battery_level_point *pb = curve;
//
//  if (batt_mV >= pb->lvl_mV) {
//    /* Measured voltage above highest point, cap at maximum. */
//    return pb->lvl_pptt;
//  }
//  /* Go down to the last point at or below the measured voltage. */
//  while ((pb->lvl_pptt > 0) && (batt_mV < pb->lvl_mV)) {
//    ++pb;
//  }
//  if (batt_mV < pb->lvl_mV) {
//    /* Below lowest point, cap at minimum */
//    return pb->lvl_pptt;
//  }
//
//  /* Linear interpolation between below and above points. */
//  const battery_level_point *pa = pb - 1;
//
//  return pb->lvl_pptt + ((pa->lvl_pptt - pb->lvl_pptt) * (batt_mV - pb->lvl_mV) / (pa->lvl_mV - pb->lvl_mV));
//}