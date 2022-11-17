/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/sensor.h>
#include "button.h"
#include "led.h"

#include "diwo_label.h"

#define DEFAULT_ADV_PERIOD_S              1                   // Период отсылки рекламы по умолчанию
#define MAX_SHOCK_TIME                    255                 // Максимальное время удержания события удара ( сек )
#define MAX_FALL_TIME                     255                 // Максимальное время удержания события падения ( сек )

#define BT_LE_ADV_NCONN_IDENTITY_1 BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY, \
						 BT_GAP_ADV_SLOW_INT_MIN, \
						 BT_GAP_ADV_SLOW_INT_MIN, \
						 NULL) 

/** Структура настроек приложения */
typedef struct {
  uint16_t adv_period;   // Период рассылки сообщений в секундах
}app_settings_s;

typedef struct {
  int last_shock;
  int last_fall;  
  bt_addr_le_t addr;
} app_var_s;

/** События приложения */
typedef enum {
  evNone    = 0,
  evTimer,
  evIrqHi,
  evIrqLo,
} events_e;

// Настройки приложения
const app_settings_s app_settings = { 
  .adv_period = DEFAULT_ADV_PERIOD_S,
};  

app_var_s app_vars = { 0 };

adv_data_s adv_data = { 
  .man_id = DIWO_ID,
  .x = 0,
  .y = 0,
  .z = 0,
  .bat = 0,
  .temp = 0,
  .hud = 0,
  .shock = 0,
  .fall = 0,
  .counter = 0,
  .reserv = { 0 },
};                // Рекламный пакет

struct bt_le_ext_adv *adv = NULL;

//uint8_t short_name[] = "DiWo";

static const struct bt_data ad[] = {
  BT_DATA_BYTES( BT_DATA_FLAGS, BT_LE_AD_NO_BREDR ),
//  BT_DATA_BYTES( BT_DATA_UUID16_ALL, 0xaa, 0xfe ),
//  BT_DATA( BT_DATA_NAME_SHORTENED, short_name, 4 ),
  BT_DATA( BT_DATA_MANUFACTURER_DATA, &adv_data, sizeof( adv_data )),
};

static const struct device *acc_lis2dw = NULL;

K_MSGQ_DEFINE( qevent, sizeof( uint8_t ), 10, 1 );
struct k_timer adv_timer; //Таймер

/** Callback по срабатыванию таймера
 */
void adv_timer_exp(struct k_timer *timer_id) {
//  k_sem_give(&timer_sem);
  uint8_t event = evTimer;
  k_msgq_put(&qevent, &event, K_NO_WAIT);
}

static void button_cb(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins) {
  //  gpio_pin_toggle_dt(&led);
  printk("Button event\r");
}

void lis12dw_trigger_handler(const struct device *dev, const struct sensor_trigger *trig) {
  //  gpio_pin_toggle_dt(&led);
  uint8_t event = evIrqHi;
  k_msgq_put(&qevent, &event, K_NO_WAIT);  
}

void main(void) {
  int err;
  
  printk("Starting DiWO Label App\n");

  const struct device *temp_dev;
//  temp_dev = device_get_binding( "temp@4000c000" );
//  temp_dev = device_get_binding( "temp" );
  temp_dev = device_get_binding( "TEMP_0" );
  if (!temp_dev) {
    printk( "error: no temp device\n" );
  }
  
  if (true != init_button( button_cb )) {
	  printk("Error init button\r");
  }	

  if (true != init_led()) {
    printk("Error init led\r");
  }
  
  acc_lis2dw = DEVICE_DT_GET_ANY(st_lis2dw12);
  if (acc_lis2dw == NULL) {
	  printk("Not found LIS2\n");
  }	

  struct sensor_value sval = { .val1 = 10, .val2 = 0 };
  sensor_attr_set( acc_lis2dw, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sval );
  
  sval.val1 = 2;
  sval.val2 = 0;
  sensor_attr_set(acc_lis2dw, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &sval); 
  
  sval.val1 = 1;
  sval.val2 = 0;
  sensor_attr_set(acc_lis2dw, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_UPPER_THRESH, &sval);   
  
  struct sensor_trigger trig;  
  
  trig.type = SENSOR_TRIG_THRESHOLD;
  trig.chan = SENSOR_CHAN_ACCEL_XYZ;
  err = sensor_trigger_set(acc_lis2dw, &trig, lis12dw_trigger_handler);  
  
	/* Initialize the Bluetooth Subsystem */
  err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
  
  k_timer_init( &adv_timer, adv_timer_exp, NULL );
  k_timer_start( &adv_timer, K_SECONDS( app_settings.adv_period ), K_SECONDS( app_settings.adv_period ) );
  
  while (1) {
    uint8_t event = 0;
    if (0 == k_msgq_get( &qevent, &event, K_FOREVER )) {
      switch (event) {
        case evTimer : {          
          err = bt_le_adv_stop();
          if (err) {
            printk( "Advertising failed to stop (err %d)\n", err );
          }
          
          adv_data.temp = ( int8_t )255;
          if (NULL != temp_dev) {
            struct sensor_value temp_value;
            sensor_sample_fetch( temp_dev );
            sensor_channel_get( temp_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp_value );
            adv_data.temp = (int8_t)temp_value.val1;
          }
           
          struct sensor_value val[3] = { 0 };
          sensor_sample_fetch( acc_lis2dw );
          sensor_channel_get( acc_lis2dw, SENSOR_CHAN_ACCEL_XYZ, val );
          
          float temp = sensor_value_to_double( &val[0] );
          adv_data.x = temp * 64;
          temp = sensor_value_to_double( &val[1] );
          adv_data.y = temp * 64;
          temp = sensor_value_to_double( &val[2] );
          adv_data.z = temp * 64;
          
          if( 0 != app_vars.last_shock ) {
            app_vars.last_shock += app_settings.adv_period;
            if( app_vars.last_shock > MAX_SHOCK_TIME ) {
              app_vars.last_shock = 0;
            }
            adv_data.shock = app_vars.last_shock;
          }
          
          if( 0 != app_vars.last_fall ) {
            app_vars.last_fall += app_settings.adv_period;
            if (app_vars.last_fall > MAX_FALL_TIME) {
              app_vars.last_fall = 0;
            }
            adv_data.fall = app_vars.last_fall;
          }
          
          adv_data.counter++;
          
          bt_le_adv_start( BT_LE_ADV_NCONN_IDENTITY_1, ad, ARRAY_SIZE( ad ), NULL, 0 );

          break;          
          
        }
        case evIrqHi : {
          app_vars.last_shock = 1;
          break;
        }
        case evIrqLo : {
          app_vars.last_fall = 1;
          break;
        }
      }
    }
  }
}
