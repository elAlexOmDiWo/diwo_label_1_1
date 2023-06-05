/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

#include "button.h"
#include "led.h"

#include "lis2dw12_reg.h"
//#include "d:\Alex\Projects\Nordic\v2.3.0\zephyr\drivers\sensor\lis2dw12\lis2dw12.h"

#include "diwo_label.h"
#include "bsp.h"
#include "wdt.h"
#include "settings.h"
#include "nfc.h"

#include "main.h"
#include "settings.h"

#define RTT_CTRL_CLEAR                "[2J"

#define RTT_CTRL_TEXT_BLACK           "[2;30m"
#define RTT_CTRL_TEXT_RED             "[2;31m"
#define RTT_CTRL_TEXT_GREEN           "[2;32m"
#define RTT_CTRL_TEXT_YELLOW          "[2;33m"
#define RTT_CTRL_TEXT_BLUE            "[2;34m"
#define RTT_CTRL_TEXT_MAGENTA         "[2;35m"
#define RTT_CTRL_TEXT_CYAN            "[2;36m"
#define RTT_CTRL_TEXT_WHITE           "[2;37m"

#define __DEBUG__                         1
#define __ENABLE_SELF_TEST_MESS__         1
//#define __ENABLE_WDT__                    1

#define DEFAULT_ADV_PERIOD_S              3                   // Период отсылки рекламы по умолчанию
#define ACC_FULL_SCALE                    160                 // мС^2

#if ( __DEBUG__ != 1 )
#define ACC_SHOCK_THRESHOLD               60                  // мС^2
#define __ENABLE_WDT__                    1
#define ODR_VALUE                         25
#else

#warning Debug. Change before release

#define ACC_SHOCK_THRESHOLD               40                  // мС^2
#define __ENABLE_WDT__                    0
#define __SEGGER_FORMAT                   1
#define ODR_VALUE                         100
#endif

#define ACC_FREFALL_DURATION              2                   // Длительность события free-fall ( 0 - 63 )
#define ACC_FREEFALL_THRESHOLD            7   //ff_thrs_7           // Порог события free-fall ( 0 - 7 )
#define MAX_SHOCK_TIME                    255                 // Максимальное время удержания события удара ( сек )
#define MAX_FALL_TIME                     255                 // Максимальное время удержания события падения ( сек )


#define BATT_READ_DELAY                   10                  // Делитель опроса батареии

#define BT_LE_ADV_NCONN_IDENTITY_1 BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY, \
						 BT_GAP_ADV_SLOW_INT_MIN*5, \
						 BT_GAP_ADV_SLOW_INT_MIN*5, \
						 NULL) 

#if ( __ENABLE_SELF_TEST_MESS__ == 1 )
#define SELF_TEST_MESS( MODULE, STATUS )        LOG_PRINTK( "SELFTEST: %-16s - %s\r", MODULE, STATUS );
#else
#define SELF_TEST_MESS( MODULE, STATUS )
#endif

typedef struct {
  int last_shock;
  int last_fall;  
  bt_addr_le_t addr;
} app_var_s;

/** События приложения */
typedef enum {
  evNone  = 0,
  evTimer,
  evIrqHi,
  evIrqLo,
} events_e;

app_settings_s app_settings = {
  .adv_period = DEFAULT_ADV_PERIOD_S,
  .power      = 0,
  .hit_threshold = 0,
  .fall_threshold = 0
};

app_var_s app_vars = { 0 };

adv_data_s adv_data = { 
  .man_id = DIWO_ID,
  .x = 0,
  .y = 0,
  .z = 0,
  .bat = 0,
  .temp = 0,
  .shock_value = 0,
  .shock = 0,
  .fall = 0,
  .counter = 0,
  .reserv = { 0 },
};

struct bt_le_ext_adv *adv = NULL;

static const struct bt_data ad[] = {
  BT_DATA_BYTES( BT_DATA_FLAGS, BT_LE_AD_NO_BREDR ),
  BT_DATA( BT_DATA_MANUFACTURER_DATA, &adv_data, sizeof( adv_data ) ),
};

static const struct device *acc_lis2dw = NULL;

K_MSGQ_DEFINE( qevent, sizeof( uint8_t ), 3, 1 );
struct k_timer adv_timer; //Таймер

//int read_regs( const struct device *dev, uint8_t start, uint8_t* data, int count ) {
//  const struct lis2dw12_device_config *cfg = dev->config;
//  stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
//
//  int err = lis2dw12_read_reg( ctx, start, data, count );
//  return err;
//}

/** Callback по срабатыванию таймера
 */
void adv_timer_exp( struct k_timer *timer_id ) {
  uint8_t event = evTimer;
  k_msgq_put( &qevent, &event, K_NO_WAIT );
}

#if 0
static void button_cb( const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins ) {
  printk( "Button event\r" );
}
#endif

/** Прерывание по превышению уровня 
*/
void lis12dw_trigger_handler( const struct device *dev, const struct sensor_trigger *trig ) {
  if (trig->type == SENSOR_TRIG_THRESHOLD) {
    uint8_t event = evIrqHi;
    k_msgq_put( &qevent, &event, K_NO_WAIT );     
  }
  else if (trig->type == SENSOR_TRIG_FREEFALL) {
    uint8_t event = evIrqLo;
    k_msgq_put( &qevent, &event, K_NO_WAIT );       
  } 
}

void lis12dw_trigger_freefall_handler( const struct device *dev, const struct sensor_trigger *trig ) {
  uint8_t event = evIrqLo;
  k_msgq_put( &qevent, &event, K_NO_WAIT );  
}

void print_device_info( void ) {
  int count = 1;
  bt_id_get( &app_vars.addr, &count );

  LOG_PRINTK( "\n" );

#if ( __SEGGER_FORMAT == 1 )
  LOG_PRINTK( RTT_CTRL_TEXT_CYAN ); 
#endif  
  
  LOG_PRINTK( "%-26s - %02x:%02x:%02x:%02x:%02x:%02x\r",
    "MAC ADDR",
    app_vars.addr.a.val[0], 
    app_vars.addr.a.val[1], 
    app_vars.addr.a.val[2], 
    app_vars.addr.a.val[3], 
    app_vars.addr.a.val[4], 
    app_vars.addr.a.val[5] );
  
  LOG_PRINTK( "%-26s - %02x%02x%02x%02x%02x%02x\r",
    "MAC ADDR REVERSE",    
    app_vars.addr.a.val[5], 
    app_vars.addr.a.val[4], 
    app_vars.addr.a.val[3], 
    app_vars.addr.a.val[2], 
    app_vars.addr.a.val[1], 
    app_vars.addr.a.val[0] ); 
  
#if ( __SEGGER_FORMAT == 1 )
  LOG_PRINTK( RTT_CTRL_TEXT_WHITE ); 
#endif

  LOG_PRINTK( "\n" );
}

void main( void ) {
  int err;

#if ( __SEGGER_FORMAT == 1 )  
  LOG_PRINTK( RTT_CTRL_CLEAR );
  LOG_PRINTK( RTT_CTRL_TEXT_WHITE );
#endif
  
  SELF_TEST_MESS( "INIT START", "OK" );
  
  init_board();
  
#if ( __ENABLE_BUTTON__ == 1 )  
  if (true != init_button( button_cb )) {
    LOG_PRINTK( "Error init button\r" );
  }	
#endif

  if (true != init_led()) {
    SELF_TEST_MESS( "LED", "ERORR" );
  }

  init_nfc();

  led_blinck( 100 );
  
  acc_lis2dw = DEVICE_DT_GET_ANY( st_lis2dw12 );
  if (acc_lis2dw == NULL) {
    SELF_TEST_MESS( "ACC", "ERROR" );
    while (1) {
      k_sleep( K_SECONDS( 1 ));
    }
  }
  SELF_TEST_MESS( "ACC", "OK" );
  
  struct sensor_value sval = { 0 };
  
  sval.val1 = ODR_VALUE;
  sval.val2 = 0;
  sensor_attr_set( acc_lis2dw, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sval );
  
/* Установка шкалы */  
  sval.val1 = ACC_FULL_SCALE;
  sval.val2 = 0;
  err = sensor_attr_set( acc_lis2dw, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &sval ); 
  if (err) {
    SELF_TEST_MESS( "ACC SCALE", "ERROR" );
  }
  
/* Установка уровня срабатывания */  
  sval.val1 = ACC_SHOCK_THRESHOLD;
  sval.val2 = 0;
  err = sensor_attr_set( acc_lis2dw, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_UPPER_THRESH, &sval );   
  if (err) {
    SELF_TEST_MESS( "ACC TRSH", "ERROR" );
  }

/* Разрешение прерывания */   
  struct sensor_trigger trig_thres;  
  trig_thres.type = SENSOR_TRIG_THRESHOLD;
  trig_thres.chan = SENSOR_CHAN_ACCEL_XYZ;
  err = sensor_trigger_set( acc_lis2dw, &trig_thres, lis12dw_trigger_handler );  
  if (err) {
    SELF_TEST_MESS( "ACC TRSH TRG", "ERROR" );
  }

  sval.val1 = ACC_FREFALL_DURATION;       // Длительность
  sval.val2 = ACC_FREEFALL_THRESHOLD;     // Порог
  err = sensor_attr_set( acc_lis2dw, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FREE_FALL, &sval );   
  if (err) {
    SELF_TEST_MESS( "ACC FF", "ERROR" );
  }
 
  struct sensor_trigger trig_fall;  
  
  trig_fall.type = 	SENSOR_TRIG_FREEFALL;
  trig_fall.chan = SENSOR_CHAN_ACCEL_XYZ;
  err = sensor_trigger_set( acc_lis2dw, &trig_fall, lis12dw_trigger_freefall_handler );  
  if (err) {
    SELF_TEST_MESS( "ACC FF TRG", "ERROR" );
  }  
  
  /* Initialize the Bluetooth Subsystem */
  err = bt_enable( NULL );
  if (err) {
    SELF_TEST_MESS( "BLE", "ERROR" );
    while (1) {
      k_sleep( K_SECONDS( 1 ));      
    }
  }
  SELF_TEST_MESS( "BLE", "OK" );

  if (0 > run_device()) {
    printk( "Error device setting\n");
    while (1) {
      k_sleep(K_SECONDS( 1 ));
    }
  }

#if ( __ENABLE_WDT__ == 1 )    
  if (true != init_wdt( 10000 )) {
    SELF_TEST_MESS( "WDT", "ERROR" );
  }
  SELF_TEST_MESS( "WDT", "OK" );
#endif
  
  k_timer_init( &adv_timer, adv_timer_exp, NULL );
  k_timer_start( &adv_timer, K_SECONDS( app_settings.adv_period ), K_SECONDS( app_settings.adv_period ) );
 
  SELF_TEST_MESS( "APP START", "OK" );   
 
  print_device_info();  
  
  while (1) {
    uint8_t event = 0;
    if (0 == k_msgq_get( &qevent, &event, K_FOREVER )) {
      switch (event) {
        case evTimer : {
          static int batt_counter = 0;
          err = bt_le_adv_stop();
#ifdef __DEBUG__
          if (err) {

            printk("Advertising failed to stop (err %d)\n", err);

          } 
          else {
            printk("Advertising ok stop\n");
          }
#endif          
          led_blinck( 1 );
          
#if ( __ENABLE_WDT__ == 1 )          
          reset_wdt();
#endif
          
          adv_data.temp = get_temp();
          
          if ( 0 == batt_counter-- ) {
            adv_data.bat = get_batt( adv_data.temp );
            batt_counter = BATT_READ_DELAY;
          }
          
          struct sensor_value val[3] = { 0 };
          sensor_sample_fetch( acc_lis2dw );
          sensor_channel_get( acc_lis2dw, SENSOR_CHAN_ACCEL_XYZ, val );
          
          float temp = sensor_value_to_double( &val[0] );
          if (temp > 2) temp = 2;
          else if (temp <= -2) temp = -2;
          adv_data.x = temp * 64;
          
          temp = sensor_value_to_double( &val[1] );          
          if (temp > 2) temp = 2;
          else if (temp <= -2) temp = -2;          
          adv_data.y = temp * 64;
          
          temp = sensor_value_to_double( &val[2] );
          if (temp > 2) temp = 2;
          else if (temp <= -2) temp = -2;          
          adv_data.z = temp * 64;
          
          if (0 != app_vars.last_shock) {
            app_vars.last_shock += app_settings.adv_period;
            if (app_vars.last_shock > MAX_SHOCK_TIME) {
              app_vars.last_shock = 0;
              adv_data.shock_value = 0;
            }
            adv_data.shock = app_vars.last_shock;
          }
          
          if (0 != app_vars.last_fall) {
            app_vars.last_fall += app_settings.adv_period;
            if (app_vars.last_fall > MAX_FALL_TIME) {
              app_vars.last_fall = 0;
            }
            adv_data.fall = app_vars.last_fall;
          }
          
          adv_data.counter++;
          
          bt_le_adv_start( BT_LE_ADV_NCONN_IDENTITY_1, ad, ARRAY_SIZE( ad ), NULL, 0 );
#ifdef __DEBUG__            
//          printk( "Send advertisment.\r" );
//          printk( "x - %02d  y - %02d  z - %02d\r", adv_data.x, adv_data.y, adv_data.z );
//          printk( "shock - %d value - %d\r", adv_data.shock, adv_data.shock_value );     
//          printk( "fall - %d\r", adv_data.fall );     
//          printk( "temp - %d  batt - %d  count - %d\n", adv_data.temp, adv_data.bat, adv_data.counter );      
#endif
          break;          
          
        }
        case evIrqHi : {
// Обработчик события по превышению
/* shock_value (uint8) - величина удара, вычисленная по формуле (x * x + y * y + z * z) / (255 * 3), 
 * вычисляется при срабатывании прерывания. При  ненулевом значении таймера shock (предыдущий удар был менее 255 секунд назад) 
 * shock_value = max( (x * x + y * y + z * z) / (255 * 3), previous_shock_value)). 
 * При обнулении таймера shock значение shock_value также обнуляется.        
*/
 
#define SHOCK_MUL                     8
          
          app_vars.last_shock = 1;
          int temp[3] = { 0 };
          int result = 0;
          int8_t val8 = 0;
        
          struct sensor_value val[3] = { 0 };
          sensor_sample_fetch( acc_lis2dw );
          sensor_channel_get( acc_lis2dw, SENSOR_CHAN_ACCEL_XYZ, val );          
         
          temp[0] = sensor_value_to_double( &val[0] ) * SHOCK_MUL;
          temp[0] *= temp[0];
          
          temp[1] = sensor_value_to_double( &val[1] ) * SHOCK_MUL;
          temp[1] *= temp[1];
          
          temp[2] = sensor_value_to_double( &val[2] ) * SHOCK_MUL;
          temp[2] *= temp[2];

          result = ( temp[0] + temp[1] + temp[2] ) / (3 * SHOCK_MUL * SHOCK_MUL );
          
          val8 = result;
          if (result > 255) {
            val8 = 255;
          }

          if (adv_data.shock_value < val8) {
            adv_data.shock_value = val8;
          }
        
#ifdef __DEBUG__        
          printk( "Threshold IRQ.\r" );          
          printk( "x - %d, y - %d, z - %d\r", temp[0], temp[1], temp[2] );
          printk( "value - %d\r", result );
          printk( "val - %d\r\n", val8 );

#endif        
          break;
        }
        case evIrqLo : {
          app_vars.last_fall = 1;
#ifdef __DEBUG__         
          printk( "FreeFall IRQ.\r" );        
#endif        
          break;
        }
      }
    }
  }
}

__weak int run_device(void) {
  return 0;
}
