/*
 * duty_cycle.h
 *
 *  Created on: Jan 30, 2020
 *      Author: abhijeet
 *      Description: Letimer initialize routines
 */
#include "em_letimer.h"
#include "sleep.h"

#ifndef TIMER_MODULE_H_
#define TIMER_MODULE_H_

#define ON_TIME 300 //user specified ON time
#define PERIOD 1000 //user specified time period

#define DEFAULT_ON_TIME_EM3 175  // default ON time EM3 175ms
#define DEFAULT_ON_TIME_EM1 2867 // default ON time EM1/EM2 175ms

#define DEFAULT_PERIOD_EM3 2117 // default time period for EM3 2250ms
#define DEFAULT_PERIOD_EM 36864 // default time period for other 2250ms

#define MAX_VALUE 65535 // maximum value of 16-bit counter
// Function to initialise letimer
void letimer_init(uint16_t on_count);
// Function to setup letimer interrupt
void letimer_interrupt_setting(void);

#endif /* TIMER_MODULE_H_ */
