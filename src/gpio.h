/*
 * gpio.h
 *
 *  Created on: Dec 12, 2018
 *      Author: Dan Walkes
 */

#ifndef SRC_GPIO_H_
#define SRC_GPIO_H_
#include <stdbool.h>

#define	LED0_port 5
#define LED0_pin 4
#define LED1_port 5
#define LED1_pin 5
#define LCD_PORT 3
#define LCD_PIN 13
#define button_port 5
#define button_pin 6
#define button_one_pin 7
#define button_one_port 5

#define GPIO_SET_DISPLAY_EXT_COMIN_IMPLEMENTED 	1
#define GPIO_DISPLAY_SUPPORT_IMPLEMENTED		1

void gpioInit();
void gpioLed0SetOn();
void gpioLed0SetOff();
void gpioLed1SetOn();
void gpioLed1SetOff();
void gpioEnableDisplay();
void gpioSetDisplayExtcomin(bool high);
#endif /* SRC_GPIO_H_ */
