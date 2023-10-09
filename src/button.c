
#include "button.h"

#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <inttypes.h>

#include "main.h"

#define LOG_LEVEL_BUTTON LOG_LEVEL_DBG

#define DEBOUNCE_INTERVAL				10

LOG_MODULE_REGISTER(button, LOG_LEVEL_BUTTON);

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE	DT_ALIAS(sw0)

#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR( SW0_NODE, gpios, { 0 });
static struct gpio_callback button_cb_data;

static struct k_work_delayable button_pressed;

static void button_cb(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins) {
  k_work_reschedule(&button_pressed, K_MSEC(DEBOUNCE_INTERVAL));
//  LOG_DBG("Button pressed irq\n");
}

static void button_pressed_fn(struct k_work *work) {
  send_event(evButton);
  LOG_DBG("Button pressed send event\n");
//  int err = callback_ctrl(false);
//
//  if (err) {
//    LOG_ERR("Cannot disable callbacks");
//    module_set_state(MODULE_STATE_ERROR);
//    return;
//  }
//
//  switch (state) {
//  case STATE_IDLE:
//    if (IS_ENABLED(CONFIG_CAF_BUTTONS_PM_EVENTS)) {
//      EVENT_SUBMIT(new_wake_up_event());
//    }
//    break;
//
//  case STATE_ACTIVE:
//    state = STATE_SCANNING;
//    k_work_reschedule(&matrix_scan, K_MSEC(DEBOUNCE_INTERVAL));
//    break;
//
//  case STATE_SCANNING:
//  case STATE_SUSPENDING:
//  default:
//    /* Invalid state */
//    __ASSERT_NO_MSG(false);
//    break;
//  }
}

bool init_button( void ) {
	int ret;

	if (!device_is_ready( button.port )) {
		LOG_ERR("Error: button device %s is not ready\n", button.port->name);
		return false;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
    LOG_ERR("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
		return false;
	}

  ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH );
	if (ret != 0) {
    LOG_ERR( "Error %d: failed to configure interrupt on %s pin %d\n", ret, button.port->name, button.pin );
		return false;
	}

  k_work_init_delayable(&button_pressed, button_pressed_fn);
  gpio_init_callback(&button_cb_data, button_cb, BIT( button.pin ));
  
	ret = gpio_add_callback(button.port, &button_cb_data);
  if (ret != 0) {
    LOG_ERR("Error add button callback - %d\n", ret );
    return false;
  }

  LOG_DBG("Set up button at %s pin %d\n", button.port->name, button.pin);
	return true;
}

int get_button_level(void) {
  return gpio_pin_get_dt(&button);
}
