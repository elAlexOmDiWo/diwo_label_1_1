
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

#include "battery.h"

/** A discharge curve specific to the power source. */
static const struct battery_level_point levels[] = {
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

  { 10000, 3000 },
  { 625, 2400 },
  { 0, 1800 },
};

static const struct device *temp_dev = DEVICE_DT_GET_ANY( nordic_nrf_temp );

void init_board ( void ) {
  if (temp_dev == NULL || !device_is_ready( temp_dev )) {
    printk( "no temperature device; using simulated data\n" );
    temp_dev = NULL;
  }
}

/** \fn get_temp Чтение температуры контроллера
* \return - температура в С или -127 в случае ошибки
*/
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

/** \brief Чтение состояния батареи
*   \return - статус батареии в % ( 0% - 100% ), или -1 в случае ошибки
*/
int8_t get_batt( int temp ) {
  int err;
  
  err = battery_measure_enable( true );
  if (err != 0) {
    printk( "Failed initialize battery measurement: %d\n", err );
    return -1;
  }  
  
  int batt_mV = battery_sample();

  err = battery_measure_enable( false );  
  if (err != 0) {
    printk( "Failed initialize battery measurement: %d\n", err );
    return -1;
  }  
  
  if ( batt_mV < 0 ) {
    printk("Failed to read battery voltage: %d\n", batt_mV  );
    return -1;
  }
  
  unsigned int batt_pptt = battery_level_pptt( batt_mV, levels );
  
  return batt_pptt / 100;
}
