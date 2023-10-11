/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#define LOG_LEVEL_MAIN LOG_LEVEL_DBG

LOG_MODULE_REGISTER( main, LOG_LEVEL_MAIN );

#define __BOARD_NAME__          "DiWo Label BLE 1.0"

#include "button.h"
#include "led.h"
#include "diwo_label.h"
#include "bsp.h"
#include "wdt.h"
#include "setting.h"
#include "nfc.h"
#include "acc.h"
#include "power.h"

#include "main.h"
#include "setting.h"

#include <nrfx.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_power.h>
#if !NRF_POWER_HAS_RESETREAS
#include <hal/nrf_reset.h>
#endif

#ifndef FW_MAJOR
#define FW_MAJOR    1
#endif

#ifndef FW_MINOR
#define FW_MINOR    0
#endif

#ifndef FW_PATCH
#define FW_PATCH    2
#endif

#define FW_VER      FW_PATCH + FW_MINOR * 100 + FW_MAJOR * 10000

#ifndef HW_MAJOR
#define HW_MAJOR    0
#endif

#ifndef HW_MINOR
#define HW_MINOR    3
#endif

#define HW_VER      HW_MINOR + HW_MAJOR * 100

#define RTT_CTRL_CLEAR                "[2J"

#define RTT_CTRL_TEXT_BLACK           "[2;30m"
#define RTT_CTRL_TEXT_RED             "[2;31m"
#define RTT_CTRL_TEXT_GREEN           "[2;32m"
#define RTT_CTRL_TEXT_YELLOW          "[2;33m"
#define RTT_CTRL_TEXT_BLUE            "[2;34m"
#define RTT_CTRL_TEXT_MAGENTA         "[2;35m"
#define RTT_CTRL_TEXT_CYAN            "[2;36m"
#define RTT_CTRL_TEXT_WHITE           "[2;37m"

//#define ACC_FREFALL_DURATION              2                   // Длительность события free-fall ( 0 - 63 )
//#define ACC_FREEFALL_THRESHOLD            7   //ff_thrs_7           // Порог события free-fall ( 0 - 7 )
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

const char* BOARD_NAME = __BOARD_NAME__;

typedef enum {
  lmInit,
  lmSleep,
  lmBeacon,
  lmDevice
} label_mode_e;

typedef struct {
  int last_shock;
  int last_fall;  
  bt_addr_le_t addr;
} app_var_s;

app_settings_s app_settings = {
  .adv_period_min = ADV_PERIOD_MIN,
  .adv_period_max = ADV_PERIOD_MAX,
  .adv_period = DEFAULT_ADV_PERIOD_S,
  .power_min = txp4p,
  .power_max = txp40m,
  .power = txp0,
  .hit_threshold = ACC_SHOCK_THRESHOLD,
  .fall_threshold = ACC_FREEFALL_THRESHOLD,
  .pass = APP_PASSWORD,
  .open = false,
  .acc_status = false,
};

app_var_s app_vars = { 0 };

label_mode_e label_mode = lmInit;

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


#if (__ENABLE_BLE__ == 1)
static const struct bt_data ad[] = {
  BT_DATA_BYTES( BT_DATA_FLAGS, BT_LE_AD_NO_BREDR ),
  BT_DATA( BT_DATA_MANUFACTURER_DATA, &adv_data, sizeof( adv_data ) ),
};
#endif

static struct k_work_delayable key_on_timer;

static void key_on_timer_fn(struct k_work *work) {
  send_event(evButtonOnEnd);
  LOG_DBG("Key on delay end\n");
}

K_MSGQ_DEFINE( qevent, sizeof( uint8_t ), 3, 1 );
struct k_timer adv_timer; //Таймер

void print_device_info( void );
int goto_deep_sleep(void);

/** \brief send event to main loop 
*   \param event
*   \return status    - 0 - OK, < 0 - error
*/
int send_event(events_e event) {
  return k_msgq_put(&qevent, &event, K_NO_WAIT);  
}

/** Callback по срабатыванию таймера
 */
void adv_timer_exp( struct k_timer *timer_id ) {
  uint8_t event = evTimer;
  k_msgq_put( &qevent, &event, K_NO_WAIT );
}
 

void main( void ) {
  int err = 0;
  int key_counter = 0;
  int time = sys_clock_cycle_get_32();  
  
#if ( __SEGGER_FORMAT == 1 )
  LOG_PRINTK( RTT_CTRL_CLEAR );
  LOG_PRINTK( RTT_CTRL_TEXT_WHITE );
#endif

#if (__ENABLE_NFC__ ==1 )
  SELF_TEST_MESS( "FIRMWARE", "Diwo Label with NFC; built on " __DATE__ " at " __TIME__ " for " CONFIG_BT_DIS_HW_REV_STR);
#else
  SELF_TEST_MESS("FIRMWARE", "Diwo Label; built on " __DATE__ " at " __TIME__ " for " CONFIG_BT_DIS_HW_REV_STR);
#endif
  //  SELF_TEST_MESS( "MCU", "DevAddr %08X %08X", NRF_FICR->DEVICEADDR[1], NRF_FICR->DEVICEADDR[0]);
  SELF_TEST_MESS( "INIT START", "OK" );

  init_board();

#if (__ENABLE_BUTTON__ == 1)
  if (true != init_button()) {
    SELF_TEST_MESS("BUTTON", "ERORR");
  }
  else {
    SELF_TEST_MESS("BUTTON", "OK");
  }
#else
  SELF_TEST_MESS("BUTTON", "DISABLE");
#endif

#if (__ENABLE_LED__ == 1 )  
  if (true != init_led()) {
    SELF_TEST_MESS( "LED", "ERORR" );
  }

  led_blinck( 100 );
#else
  SELF_TEST_MESS("LED", "DISABLE");
#endif

#if ( __ENABLE_ACC__ == 1 )  
  if (true != init_acc()) {
    SELF_TEST_MESS("ACC", "ERROR");
    app_settings.acc_status = false;
  }
  else {
    SELF_TEST_MESS("ACC", "OK");
  }
#else
  SELF_TEST_MESS("ACC", "DISABLE");
#endif

#if (__ENABLE_BLE__ == 1 )  
  if ( 0 != bt_enable( NULL )) {
      SELF_TEST_MESS( "BLE", "ERROR" );
  }
  else {
    SELF_TEST_MESS( "BLE", "OK" );
  }
#else
  SELF_TEST_MESS( "BLE", "DISABLE" );  
#endif
  
#if ( __ENABLE_NFC__ == 1 ) 
  if (0 != init_nfc()) {
    SELF_TEST_MESS( "NFC", "ERROR" );
  }
  else {
    SELF_TEST_MESS( "NFC", "OK" );
  }
#else
  SELF_TEST_MESS( "NFC", "DISABLE" );
#endif

#if (__ENABLE_WDT__ == 1)
  if (true != init_wdt(10000)) {
    SELF_TEST_MESS("WDT", "ERROR");
  }
  SELF_TEST_MESS("WDT", "OK");
#else
  SELF_TEST_MESS("WDT", "DISABLE");
#endif
  
  SELF_TEST_MESS( "APP START", "OK" );

#if (__ENABLE_BLE__ == 1)
  print_device_info();
#endif
  
#if (__ENABLE_DEEP_SLEEP__ != 0)  
  uint32_t reas = get_reset_reason();
  if( reas & NRF_POWER_RESETREAS_OFF_MASK) {
    LOG_DBG("System resume\n");
    while (1) {
      uint8_t event = 0;
      if (0 == k_msgq_get(&qevent, &event, K_SECONDS(2))) {
        if (event == evButton) {
          if (1 == get_button_level()) {
            int event_time = sys_clock_tick_get_32();
            int dtime = event_time - time;
            time = event_time;
            LOG_DBG("Key delay - %d\n", dtime);
            if ((dtime < KEY_EVENT_MIN) || (dtime > KEY_EVENT_MAX)) {
              LOG_DBG("Key delay too short\n");
              goto_deep_sleep();
            }
            if (key_counter++ >= 3) {
              LOG_DBG("Key pass enter\n");
#if (__ENABLE_LED__ == 1)
              led_blinck(300);
#endif
              break;
            }
          }
        }
      }
      else {
        LOG_DBG("Key delay too long\n");
        goto_deep_sleep();
        }
    }
  }
  else if (reas & NRF_POWER_RESETREAS_DOG_MASK) {
    LOG_DBG("Reset watchdog source\n");
  }
#if (__ENABLE_NFC__ == 1)  
  else if (reas & NRF_POWER_RESETREAS_NFC_MASK) {
    LOG_DBG("Resume by NFC\n");
  }
#endif
  else {
    if (IS_ENABLED(CONFIG_APP_RETENTION)) {
      LOG_ERR("CONFIG_APP_RETENTION\n");
    }
    goto_deep_sleep();
  } 
#endif
  
  k_timer_init(&adv_timer, adv_timer_exp, NULL);
  k_timer_start( &adv_timer, K_SECONDS( app_settings.adv_period ), K_SECONDS( app_settings.adv_period ) );

  k_work_init_delayable(&key_on_timer, key_on_timer_fn);
  label_mode = lmBeacon;
  key_counter = 0;
  
  while (1) {
    uint8_t event = 0;
    if (0 == k_msgq_get( &qevent, &event, K_FOREVER )) {
      switch (event) {
        case evTimer : {
          static int batt_counter = 0;
#if (__ENABLE_BLE__ == 1)          
          err = bt_le_adv_stop();

          if (err) {
            LOG_ADV_DBG( "Advertising stop FAULT(err %d)\n", err );
          } 
          else {
            LOG_ADV_DBG( "Advertising stop OK\n" );
          }       
#endif
          
#if (__ENABLE_LED__ == 1 )        
          led_blinck( 1 );
#endif 
          
#if ( __ENABLE_WDT__ == 1 )          
          reset_wdt();
#endif
        
          adv_data.temp = get_temp();
          
          if ( 0 == batt_counter-- ) {
            adv_data.bat = get_batt( adv_data.temp );
            batt_counter = BATT_READ_DELAY;
          }
#if ( __ENABLE_ACC__ == 1 )
          if (app_settings.acc_status == true) {
            read_adv_acc_data(&adv_data);
          }
#else
          adv_data.x = adv_data.y = adv_data.z = 0;
#endif
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
#if (__ENABLE_BLE__ != 0)
          if (0 != (err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY_1, ad, ARRAY_SIZE(ad), NULL, 0))) {
            LOG_ADV_DBG("Advertising start FAULT(err %d)\n", err);
          }
          else {
            LOG_ADV_DBG("Advertising start OK\n");
          }

          LOG_ADV_DBG( "\rSend advertisment.\r" );
          LOG_ADV_DBG( "x - %02d  y - %02d  z - %02d\r", adv_data.x, adv_data.y, adv_data.z );
          LOG_ADV_DBG( "shock - %d value - %d\r", adv_data.shock, adv_data.shock_value );
          LOG_ADV_DBG( "fall - %d\r", adv_data.fall );
          LOG_ADV_DBG( "temp - %d  batt - %d\rpacket counter - %d\r", adv_data.temp, adv_data.bat, adv_data.counter );
#endif          
          break;          
        }
        case evIrqHi: {
          // Обработчик события по превышению
          /* shock_value (uint8) - величина удара, вычисленная по формуле (x * x + y * y + z * z) / (255 * 3), 
 * вычисляется при срабатывании прерывания. При  ненулевом значении таймера shock (предыдущий удар был менее 255 секунд назад) 
 * shock_value = max( (x * x + y * y + z * z) / (255 * 3), previous_shock_value)). 
 * При обнулении таймера shock значение shock_value также обнуляется.        
*/

#define SHOCK_MUL 8

          app_vars.last_shock = 1;
          int temp[3] = {0};
          int result = 0;
          int8_t val8 = 0;

          struct sensor_value val[3] = {0};
#if (__ENABLE_ACC__ == 1)
          if (app_settings.acc_status == true) {
            read_sensor_value(val);
          }
#endif
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
#if (__ENABLE_ACC__ == 1)
          LOG_DBG( "Threshold IRQ.\r" );
          LOG_DBG( "x - %d, y - %d, z - %d\r", temp[0], temp[1], temp[2] );
          LOG_DBG( "value - %d\r", result );
          LOG_DBG( "val - %d\r\n", val8 );  
#endif          
          break;
        }
        case evIrqLo : {
          app_vars.last_fall = 1;
          LOG_DBG( "FreeFall IRQ.\r" );               
          break;
        }
        case evButton: {
          switch (label_mode ) {
            case lmBeacon: {
              int result;
              static int counter = 0;
              int value = get_button_level();
              if (value == 1) {
                k_work_reschedule(&key_on_timer, K_TICKS( KEY_EVENT_MAX));
                int event_time = sys_clock_tick_get_32();
                int dtime = event_time - time;
                time = event_time;
                LOG_DBG("Key delay - %d\n", dtime);
                if (key_counter++) {
                  if ((dtime < KEY_EVENT_MIN) || (dtime > KEY_EVENT_MAX)) {
                    LOG_DBG("Key delay too short\n");
                    key_counter = 0;
                    break;
                  }                  
                  if (key_counter >= 3) {
                    
                    k_timer_stop(&adv_timer);
                    err = bt_le_adv_stop();
                    LOG_DBG("Key pass enter\n");
#if (__ENABLE_LED__ == 1)
                    led_blinck(300);
#endif                    
                    if (0 <= (result = run_device())) {
                      
                    }
                    app_settings.open = false;
                    k_timer_start(&adv_timer, K_SECONDS(app_settings.adv_period), K_SECONDS(app_settings.adv_period));                    
                  }
                }
              }
              else {
                LOG_DBG("Key release - %d\r", counter++);
              } 
              break;
            }
            case lmDevice: {
              label_mode = lmBeacon;
              break;
            }              
          }
          break;
        }
        case evButtonOnEnd: {
          if (key_counter != 0) {
            LOG_DBG("Key delay too long\n");
            key_counter = 0;
          }
          break;
        }
        case evCmdPowerOff: {
          break;
        }
      }
    }
  }
}

__weak int run_device(void) {
  return 0;
}

__weak bool init_button( void ) {
  return true;
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

int goto_deep_sleep(void) {
#if (__ENABLE_DEEP_SLEEP__ != 0)
  nrf_gpio_cfg_input(NRF_DT_GPIOS_TO_PSEL(DT_ALIAS(sw0), gpios), NRF_GPIO_PIN_PULLUP);
  nrf_gpio_cfg_sense_set(NRF_DT_GPIOS_TO_PSEL(DT_ALIAS(sw0), gpios), NRF_GPIO_PIN_SENSE_LOW);

  LOG_DBG("System off\n");
  while (log_data_pending()) {
    k_sleep(K_MSEC(100));
  }
  pm_policy_state_lock_get(PM_STATE_SOFT_OFF, PM_ALL_SUBSTATES);
  pm_state_force(0u, &(struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});
  k_sleep(K_MSEC(1));
  return 0;
#else
  SELF_TEST_MESS("DEEP SLEEP", "DISABLE");
  return -1;
#endif
}

/** \mainpage
* По включению устройство проходит начальное тестирование и уходит в глубокий сон.<br>
* По нажатию кнопки устройство выходит из сна и переходит в режим метки.<br>
* По следующему нажатию переходит в режим настройки для изменения параметров.<br>
* На подключение к устройству действует таймаут 10 сек - рабочий режим / 100 сек - отладка.<br>
* При подключении у устройству становиться возможной запись параметров.<br>
* Таймаут операции - 120 сек - рабочий режим / 1200 сек - отладка.<br>
* Запись в 5-й по счёту пункт пароля ( 01 02 03 04 05 06 07 08 - отладочный, рабочий - отдельно ) разрешает управлять устройством.<br>
* Запись команды 95 в 6-й пункт приводит к сбросу устройства и переходу в режим сна.<br>
*/