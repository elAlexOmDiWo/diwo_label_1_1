/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/sensor.h>
#include "button.h"
#include "led.h"

static uint8_t mfg_data[] = { 0xff, 0xff, 0x00 };

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, 3),
};

static const struct device *acc_lis2dw = NULL;

static void button_cb(const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins) {
  gpio_pin_toggle_dt(&led);
  printk("Button event\r");
}

void lis12dw_trigger_handler(const struct device *dev, const struct sensor_trigger *trig) {
  gpio_pin_toggle_dt(&led);
}

void main(void)
{
  struct bt_le_ext_adv *adv;
  int err;

  printk("Starting Periodic Advertising Demo\n");

  if (true != init_button( button_cb )) {
	printk("Error init button\r");
  }	

  if (true != init_led()) {
    printk("Error init led\r");
  }
  
  acc_lis2dw = DEVICE_DT_GET_ANY(st_lis2dw12);
  if (acc_lis2dw == NULL) {
	printk("Not found LIS2\n");
  }	

  struct sensor_value sval = { .val1 = 25, .val2 = 0 };
  sensor_attr_set( acc_lis2dw, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &sval );
  
  sval.val1 = 8;
  sval.val2 = 0;
  sensor_attr_set(acc_lis2dw, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &sval); 
  
  sval.val1 = 4;
  sval.val2 = 0;
  sensor_attr_set(acc_lis2dw, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_UPPER_THRESH, &sval);   
  
  struct sensor_trigger trig;  
  
  trig.type = SENSOR_TRIG_THRESHOLD;
  trig.chan = SENSOR_CHAN_ACCEL_XYZ;
  err = sensor_trigger_set(acc_lis2dw, &trig, lis12dw_trigger_handler);  
  
	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	/* Create a non-connectable non-scannable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL, &adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		printk("Failed to set periodic advertising parameters"
		       " (err %d)\n", err);
		return;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(adv);
	if (err) {
		printk("Failed to enable periodic advertising (err %d)\n", err);
		return;
	}
  
  while (1) {
    k_sleep(K_MSEC( 990));
    gpio_pin_set_dt( &led, 1 );
    k_sleep(K_MSEC( 10 ));
    gpio_pin_set_dt( &led, 0 );    
  }

	while (true) {
		printk("Start Extended Advertising...");
		err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
		if (err) {
			printk("Failed to start extended advertising "
			       "(err %d)\n", err);
			return;
		}
		printk("done.\n");

		for (int i = 0; i < 3; i++) {
			k_sleep(K_SECONDS(1));

			mfg_data[2]++;

  			gpio_pin_toggle_dt(&led);
  		
			printk("Set Periodic Advertising Data...");
			err = bt_le_per_adv_set_data(adv, ad, ARRAY_SIZE(ad));
			if (err) {
				printk("Failed (err %d)\n", err);
				return;
			}
			printk("done.\n");
		}

		k_sleep(K_SECONDS(1));

		printk("Stop Extended Advertising...");
		err = bt_le_ext_adv_stop(adv);
		if (err) {
			printk("Failed to stop extended advertising "
			       "(err %d)\n", err);
			return;
		}
		printk("done.\n");

		k_sleep(K_SECONDS(10));
	}
}
