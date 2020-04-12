/*
 * clock_init.c
 *
 *  Created on: Jan 30, 2020
 *      Author: abhijeet
 *      Description: Clock init, clock select and prescalar select functions
 */

#include "clock_init.h"
#include "sleep.h"
#include "em_letimer.h"
// Function to initialise lfxo
void lfxo_init(uint16_t DIV)
{
	//enable oscillator
	CMU_OscillatorEnable(cmuOsc_LFXO,true,true);
	// select ULFRCO as clock source for branch LFACLK
	CMU_ClockSelectSet(cmuClock_LFA,cmuSelect_LFXO);
	CMU_ClockDivSet(cmuClock_LETIMER0,DIV);
}

// Function to initialise ULFRCO
void ulfrco_init(void)
{
	//enable oscillator
	CMU_OscillatorEnable(cmuOsc_ULFRCO,true,true);
	// select ULFRCO as clock source for branch LFACLK
	CMU_ClockSelectSet(cmuClock_LFA,cmuSelect_ULFRCO);
}

// Function to enable clock for LETIMER0 and LFACLK
void Lfa_Letimer0_enable(void)
{
	  CMU_ClockEnable(cmuClock_LFA,true);
	  CMU_ClockEnable(cmuClock_LETIMER0,true);
}

// Function to select clock based on energy modes
uint16_t clock_select(const SLEEP_EnergyMode_t EM_s,uint16_t time_s)
{
	uint16_t ps = 1;
	if(EM_s == sleepEM3)
	 {
		 ulfrco_init();
	 }
	 if(EM_s == sleepEM1 || EM_s == sleepEM2 || EM_s == sleepEM0)
	 {
		 ps = prescalar_select(time_s);
		 lfxo_init(ps);
	 }
return ps;
}

// Function to change prescalar value dynamically
uint16_t prescalar_select(uint16_t time_t)
{
	uint16_t prescalar = 1;
    uint32_t count;

    count = time_t * (32768/(prescalar));
    while(count/1000>0xFFFF)
    {
    	prescalar = prescalar*2;
    	count = time_t * (32768/prescalar);
    }
    return prescalar;
}

// Function to convert time into counts
uint32_t count_cvt(uint16_t prescaler,uint16_t time_t, const SLEEP_EnergyMode_t EMS)
{
uint32_t count;
if(EMS == sleepEM0 || EMS == sleepEM1 || EMS == sleepEM2)
{
	count = time_t * (32768/(prescaler));
	count = count/1000;
}
else if(EMS == sleepEM3)
{
	count = time_t;
}

return count;
}

// Takes input in terms of 16bit count value not time
void timerWaitUs(uint32_t us_wait)
{
	uint32_t current_time = LETIMER_CounterGet(LETIMER0);
	while(LETIMER_CounterGet(LETIMER0) != (current_time-us_wait));
}

void NB_Timer(uint32_t wait)
{
	uint32_t value;
	value = (LETIMER_CounterGet(LETIMER0)-(count_cvt(pre_scalar,wait,sleepEM3)));
	LETIMER_CompareSet(LETIMER0,1,value);

}

uint32_t timerGetRunTimeMilliseconds(const SLEEP_EnergyMode_t EM_Select)
{
	//float time_t;
	uint32_t milli;
	uint32_t time_v;

	milli = 65535 - LETIMER_CounterGet(LETIMER0);
	time_v = milli + secs;
	//time_t = (float)(milli * ((pre_scalar*1000)/32768));
	//time_v = time_t;
	//return time_v;
	return time_v;
}




