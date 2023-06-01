
/** \file led.c
*   \author   Alex  
*   \date 1.11.2022
*/

#include "led.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define LED0_NODE DT_ALIAS(led0)
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

struct k_timer led_timer; //Таймер

k_timeout_t led_period = { 0 };

void led_timer_exp(struct k_timer *timer_id) {
  k_timer_stop( &led_timer );
  gpio_pin_set_dt( &led, 0 );
}

bool init_led(void) {
  int err;

  if (!device_is_ready(led.port)) {
    printk("Led port init failed\n");
    return false;
  }

  err = gpio_pin_configure_dt( &led, GPIO_OUTPUT_ACTIVE );
  if (err < 0) {
    printk("Led pin init failed (err %d)\n", err);
    return false;
  }

  gpio_pin_set_dt( &led, 0 );

  led_period = K_MSEC(100);
  
  k_timer_init( &led_timer, led_timer_exp, NULL );
  k_timer_start( &led_timer, led_period, K_MSEC( 0 )) ;  
  
  return true;
}

void led_blinck( int period_ms ) {
  gpio_pin_set_dt( &led, 1 );
  k_timer_start( &led_timer, K_MSEC( period_ms ), K_MSEC( 0 ) ) ;  
}