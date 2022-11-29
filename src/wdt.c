
/** \file		wdt.c
*		\author	Alex
*		\date		27.11.2022
*
*	Реализация WDT для приложения
*/

#include "wdt.h"

#include <stdint.h>
#include <stdbool.h>

//#include <zephyr/bluetooth/bluetooth.h>
//#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

  
#ifndef WDT_MAX_WINDOW
//#define WDT_MAX_WINDOW  10000U
#endif  

struct device *wdt = NULL;
int wdt_channel_id;

bool init_wdt( uint32_t period ) {
  int err;
  
  wdt = DEVICE_DT_GET( DT_ALIAS( watchdog0 ) );  
  if (!device_is_ready( wdt )) {
    LOG_PRINTK( "WDT Device not ready" );
    return false;
  }

  struct wdt_timeout_cfg wdt_config = {
    /* Reset SoC when watchdog timer expires. */
    .flags = WDT_FLAG_RESET_SOC,

    /* Expire watchdog after max window */
    .window.min = 0U,
    .window.max = period,
  };

  wdt_channel_id = wdt_install_timeout( wdt, &wdt_config );
  if (wdt_channel_id == -ENOTSUP) {
    /* IWDG driver for STM32 doesn't support callback */
    printk( "Callback support rejected, continuing anyway\n" );
    wdt_config.callback = NULL;
    wdt_channel_id = wdt_install_timeout( wdt, &wdt_config );
  }
  if (wdt_channel_id < 0) {
    LOG_PRINTK( "Watchdog install error\n" );
    return false;
  }  
  
  err = wdt_setup( wdt, WDT_OPT_PAUSE_HALTED_BY_DBG );
  if (err < 0) {
    LOG_PRINTK( "Watchdog setup error\n" );
    return false;
  }  
  
  return true;
}

void reset_wdt( void ) {
  wdt_feed( wdt, wdt_channel_id );
}