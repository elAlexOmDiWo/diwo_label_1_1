
/** \file led.c
*   \author   Alex  
*   \date 1.11.2022
*/

#include "led.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define LED0_NODE DT_ALIAS(led0)
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

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
  
  return true;
}