/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/sensor.h>
#include "button.h"
#include "led.h"

#define DEFAULT_ADV_PERIOD_S              1                   // Период отсылки рекламы по умолчанию
#define MAX_SHOCK_TIME                    255                 // Максимальное время удержания события удара ( сек )
#define MAX_FALL_TIME                     255                 // Максимальное время удержания события падения ( сек )

/** Структура настроек приложения */
typedef struct {
  uint16_t adv_period;   // Период рассылки сообщений в секундах
}app_settings_s;

typedef struct {
  int last_shock;
  int last_fall;  
  bt_addr_le_t addr;
} app_var_s;

/** Описание рекламного пакета */
typedef struct {
  int8_t x;
  int8_t y;
  int8_t z;
  uint8_t bat;
  int8_t temp;
  uint8_t hud;
  uint8_t shock;
  uint8_t fall; 
} adv_data_s;

/** События приложения */
typedef enum {
  evNone    = 0,
  evTimer,
  evIrqHi,
  evIrqLo,
  evAdvSent,
  evBleStart,
} events_e;

// Настройки приложения
const app_settings_s app_settings = { 
  .adv_period = DEFAULT_ADV_PERIOD_S,
};  

app_var_s app_vars = { 0 };

adv_data_s adv_data = { 0 };                // Рекламный пакет

struct bt_le_ext_adv *adv = NULL;

static const struct bt_data ad[] = {
  BT_DATA(BT_DATA_MANUFACTURER_DATA, &adv_data, 8),
};

static struct bt_le_ext_adv_start_param start_param = {
  .num_events = 1,
  .timeout = 10000,
};

static const struct device *acc_lis2dw = NULL;

K_MSGQ_DEFINE( qevent, sizeof( uint8_t ), 10, 1 );
struct k_timer adv_timer; //Таймер

/** Callback по отправке данных */
void bt_adv_sent_cb( struct bt_le_ext_adv *adv, struct bt_le_ext_adv_sent_info *info );

// Структура callback-функций ble adv
struct bt_le_ext_adv_cb ble_adv_cb = { 
  .sent       = bt_adv_sent_cb,
  .connected  = NULL,
  .scanned    = NULL
};

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

static void bt_ready(int err) {
  
//#define BT_LE_EXT_ADV_NCONN_IDENTITY_PARAM \
//		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV | \
//				BT_LE_ADV_OPT_USE_IDENTITY, \
//				BT_GAP_ADV_SLOW_INT_MIN, \
//				BT_GAP_ADV_SLOW_INT_MAX, NULL)  
  
  /* Create a non-connectable non-scannable advertising set */
//  err = bt_le_ext_adv_create( BT_LE_EXT_ADV_NCONN_IDENTITY_PARAM, &ble_adv_cb, &adv );    
  err = bt_le_ext_adv_create( BT_LE_EXT_ADV_NCONN_IDENTITY, &ble_adv_cb, &adv );
  if (err) {
    printk( "Failed to create advertising set (err %d)\n", err );
    return;
  }

  /* Set periodic advertising parameters */
  struct bt_le_per_adv_param param = {
    .interval_min = app_settings.adv_period * 1000,
    .interval_max = app_settings.adv_period * 1000,
    .options = BT_LE_ADV_OPT_NO_2M,
  };
  err = bt_le_per_adv_set_param( adv, &param );  
  if (err) {
    printk("Failed to set periodic advertising parameters" " (err %d)\n", err  );
    return;
  }

  err = bt_le_per_adv_set_data( adv, ad, ARRAY_SIZE( ad ));  
  if (err) {
    printk( "Failed to set periodic advertising data (err %d)\n", err );    
  }
  
  /* Enable Periodic Advertising */
  err = bt_le_per_adv_start( adv );
  if (err) {
    printk( "Failed to enable periodic advertising (err %d)\n", err );
    return;
  }

//  err = bt_le_ext_adv_start( adv, BT_LE_EXT_ADV_START_DEFAULT );  
  err = bt_le_ext_adv_start( adv, &start_param );  
  if (err) {
    printk( "Failed to enable periodic advertising (err %d)\n", err );
    return;
  }
  
  int addr_count = 1;
  bt_id_get( &app_vars.addr, &addr_count );  
  
  uint8_t event = evBleStart;
  k_msgq_put( &qevent, &event, K_NO_WAIT );
}

/** Callback по отправке данных */
void bt_adv_sent_cb( struct bt_le_ext_adv *adv, struct bt_le_ext_adv_sent_info *info ) {
//  gpio_pin_toggle_dt( &led );
//  bt_le_per_adv_stop( adv );
  uint8_t event = evAdvSent;
  k_msgq_put( &qevent, &event, K_NO_WAIT );   
}

void main(void) {
  int err;

  printk("Starting Periodic Advertising Demo\n");

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
  
  sval.val1 = 8;
  sval.val2 = 0;
  sensor_attr_set(acc_lis2dw, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &sval); 
  
  sval.val1 = 4;
  sval.val2 = 0;
  sensor_attr_set(acc_lis2dw, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_UPPER_THRESH, &sval);   
  
  struct sensor_trigger trig;  
  
  trig.type = SENSOR_TRIG_THRESHOLD;
  trig.chan = SENSOR_CHAN_ACCEL_XYZ;
  err = sensor_trigger_set(acc_lis2dw, &trig, lis12dw_trigger_handler);  
  
	/* Initialize the Bluetooth Subsystem */
  err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
  
  k_timer_init( &adv_timer, adv_timer_exp, NULL );
  
  while (1) {
    uint8_t event = 0;
    if (0 == k_msgq_get( &qevent, &event, K_FOREVER )) {
      switch (event) {
        case evBleStart : {
          k_timer_start( &adv_timer, K_SECONDS( app_settings.adv_period ), K_SECONDS( app_settings.adv_period )); 
          break;
        }
        case evTimer : {
          
          static int counter = 0;
          uint8_t* k = (uint8_t*)&adv_data;
          for (int i = 0; i < sizeof( adv_data ); i++) {
            k[i] = (uint8_t)counter;
          }
          counter++;
          err = bt_le_per_adv_set_data( adv, ad, ARRAY_SIZE( ad ) );  
          if (err) {
            printk( "Failed to set periodic advertising data (err %d)\n", err );    
          }          
//          err = bt_le_ext_adv_start( adv, &start_param );
          break;
          
          struct sensor_value val[3];// = { 0 };
          sensor_sample_fetch( acc_lis2dw );
          if (0 == sensor_channel_get( acc_lis2dw, SENSOR_CHAN_ACCEL_XYZ, val )) {
//            gpio_pin_toggle_dt( &led );            
          }
          
          //k_sched_lock();
          
          adv_data.x++;// = val[0].val2;
          adv_data.y++;// = val[1].val2;
          adv_data.z++;// = val[2].val2;
          
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
          
          //k_sched_unlock();
          
          err = bt_le_per_adv_set_data( adv, ad, ARRAY_SIZE( ad ) );  
          if (err) {
            printk( "Failed to set periodic advertising data (err %d)\n", err );    
          }
          err = bt_le_ext_adv_start( adv, &start_param );
          
          
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
