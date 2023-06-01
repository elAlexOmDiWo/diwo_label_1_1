
#include "button.h"

#include <stdbool.h>

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE	DT_ALIAS(sw0)

#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR( SW0_NODE, gpios, { 0 });
static struct gpio_callback button_cb_data;

bool init_button(gpio_callback_handler_t handler) {
	int ret;

	if (!device_is_ready( button.port )) {
		printk("Error: button device %s is not ready\n",
			button.port->name);
		return false;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
			ret,
			button.port->name,
			button.pin);
		return false;
	}

	ret = gpio_pin_interrupt_configure_dt( &button, GPIO_INT_EDGE_TO_ACTIVE );
	if (ret != 0) {
		printk( "Error %d: failed to configure interrupt on %s pin %d\n", ret, button.port->name, button.pin );
		return false;
	}

	gpio_init_callback( &button_cb_data, handler, BIT( button.pin ));
  
	gpio_add_callback(button.port, &button_cb_data);
  
//	printk("Set up button at %s pin %d\n", button.port->name, button.pin);

	return true;
}
