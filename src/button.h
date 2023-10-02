#pragma once

#include <stdbool.h>
#include <zephyr/drivers/gpio.h>

bool init_button( void );

int get_button_level(void);