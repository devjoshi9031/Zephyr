/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/sensor.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <usb/usb_device.h>
#include <drivers/uart.h>


/*
 * Get a device structure from a devicetree node with compatible
 * "bosch,bme280". (If there are multiple, just pick one.)
 */
static const struct device *get_bme280_device(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(bosch_bme280);

	if (dev == NULL) {
		/* No such node, or the node does not have status "okay". */
		printk("\nError: no device found.\n");
		return NULL;
	}

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return NULL;
	}

	printk("Found device \"%s\", getting sensor data\n", dev->name);
	return dev;
}

void main(void)
{
	const struct device *dev = get_bme280_device();
	const struct device *dev_led, *dev_usb;

        /* USB serial setup */
  dev_usb = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
        uint32_t dtr = 0;

        if (usb_enable(NULL)) {
                return;
        }

        while (!dtr) {
                uart_line_ctrl_get(dev_usb, UART_LINE_CTRL_DTR, &dtr);
        }

	if (dev == NULL) {
		return;
	}
	printk("It went uptill here!\n");
	while (1) {
		struct sensor_value temp, press, humidity;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);
		sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);

		printk("temp: %d.%06d; press: %d.%06d; humidity: %d.%06d\n",
		      temp.val1, temp.val2, press.val1, press.val2,
		      humidity.val1, humidity.val2);

		k_sleep(K_MSEC(1000));
	}
}
