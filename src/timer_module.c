/*
 * duty_cycle.c
 *
 *  Created on: Jan 30, 2020
 *      Author: abhijeet
 *      Description: Letimer initialize routines
 */
#include "timer_module.h"

// function to initialize letimer and set value of COMP0
void letimer_init(uint16_t on_count)
{
	const LETIMER_Init_TypeDef letimerInit =
	{
	.enable = false,  //Start counting when init completed
	.debugRun = false,  //Counter shall not keep running during debug halt.
	.comp0Top = false,  //Load COMP0 register into CNT when counter underflows. COMP is used as TOP
	.bufTop = false,  //Don't load COMP1 into COMP0 when REP0 reaches 0.
	.out0Pol = 0,  //Idle value for output 0.
	.out1Pol = 0,  //Idle value for output 1.
	.ufoa0 = letimerUFOANone,  //PwM output on output 0
	.ufoa1 = letimerUFOANone,  //No output on output 1
	.repMode = letimerRepeatFree  //Count while REP != 0
	};

	 uint32_t on_time;
	 LETIMER_Init(LETIMER0,&letimerInit);
     on_time = (MAX_VALUE - on_count);
     LETIMER_CompareSet(LETIMER0,0,on_time); // 175ms = 0xFF50 in ULFRCO, == FA65
	 //NVIC_EnableIRQ(LETIMER0_IRQn);
}

// function to set letimer interrupts
void letimer_interrupt_setting(void)
{
	LETIMER_IntEnable(LETIMER0,0x07); // Enable COM0 and UF Interrupt
	LETIMER_Enable(LETIMER0,true);
}




