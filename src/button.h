#pragma once

#include <stdbool.h>
#include <zephyr/drivers/gpio.h>

bool init_button(gpio_callback_handler_t handler);