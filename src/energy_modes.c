/*
 * energy_modes.c
 *
 *  Created on: Jan 31, 2020
 *      Author: abhijeet
 *      Description: sleep modes selecting function
 */
#include"energy_modes.h"

// function to block modes based on selected energy modes
void sleep_block(const SLEEP_EnergyMode_t EM_s)
{
	if(EM_s==sleepEM1)
	{
		SLEEP_SleepBlockBegin(sleepEM2);// sleep won't enter EM2/EM3/EM4
	}
	if(EM_s==sleepEM2)
	{
		SLEEP_SleepBlockBegin(sleepEM3);// sleep won't enter EM3/EM4
	}

}
