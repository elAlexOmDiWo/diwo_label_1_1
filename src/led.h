
/** \file led.h
*   \author   Alex  
*   \date 1.11.2022
*/

#pragma once

#include <stdbool.h>

#include <zephyr/drivers/gpio.h>

extern const struct gpio_dt_spec led;

bool init_led(void);

void led_blinck( int period_ms );