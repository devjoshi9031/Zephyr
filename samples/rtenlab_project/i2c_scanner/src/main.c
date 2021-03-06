/*
 * Copyright (c) 2018 Tavish Naruka <tavishnaruka@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/i2c.h>
#include <usb/usb_device.h>
#include <drivers/uart.h>

#ifdef ARDUINO_I2C_LABEL
#define I2C_DEV ARDUINO_I2C_LABEL
#else
#define I2C_DEV "I2C_0"
#endif

/**
 * @file This app scans I2C bus for any devices present
 */

void main(void)
{
	struct device *i2c_dev, *dev_usb;
	uint32_t dtr=0;

	dev_usb = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	if(usb_enable(NULL))
		return;
	
	while(!dtr)
		uart_line_ctrl_get(dev_usb,UART_LINE_CTRL_DTR, &dtr);

	printk("Starting i2c scanner...\n");

	i2c_dev = device_get_binding(I2C_DEV);
	if (!i2c_dev) {
		printk("I2C: Device driver not found.\n");
		return;
	}
	printk("Got device binding\n");
	for (uint8_t i = 4; i <= 0x77; i++) {
		struct i2c_msg msgs[1];
		uint8_t dst;

		/* Send the address to read from */
		msgs[0].buf = &dst;
		msgs[0].len = 0U;
		msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;
		
		if (i2c_transfer(i2c_dev, &msgs[0], 1, i)==0) {
			printk("0x%2x FOUND\n", i);
		}
	}
}