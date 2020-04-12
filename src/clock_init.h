/*
 * clock_init.h
 *
 *  Created on: Jan 30, 2020
 *      Author: abhijeet
 *      Description: Clock init, clock select and prescalar select functions
 */

#ifndef CLOCK_INIT_H_
#define CLOCK_INIT_H_

#include "em_cmu.h"
#include "stdbool.h"
#include "sleep.h"

#define TIMER_SUPPORTS_1HZ_TIMER_EVENT	1

extern uint16_t pre_scalar;
extern uint32_t secs;

// Function to initialise ULFRCO
void ulfrco_init(void);
// Function to initilaise LFXO
void lfxo_init(uint16_t DIV);
// Function to enable clock to letimer
void Lfa_Letimer0_enable(void);
// Function to select clock on the basis of energy mode selected
uint16_t clock_select(const SLEEP_EnergyMode_t EM_s,uint16_t time_s);
// Function to dynamically select prescalar
uint16_t prescalar_select(uint16_t time_t);
// Function to convert milliseconds to count
uint32_t count_cvt(uint16_t prescaler,uint16_t time_t, const SLEEP_EnergyMode_t EMS);
// Function to select clock based on energy modes
uint16_t clock_select(const SLEEP_EnergyMode_t EM_s, uint16_t time_s);
// Delay Function
void timerWaitUs(uint32_t us_wait);
// Non-blocking timer
void NB_Timer(uint32_t wait);
//function to log time
uint32_t timerGetRunTimeMilliseconds(const SLEEP_EnergyMode_t EM_Select);

#endif /* CLOCK_INIT_H_ */
