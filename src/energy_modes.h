/*
 * energy_modes.h
 *
 *  Created on: Jan 31, 2020
 *      Author: abhijeet
 *      Description: sleep modes selecting function
 */
#include"sleep.h"


#ifndef SRC_ENERGY_MODES_H_
#define SRC_ENERGY_MODES_H_
// function to block modes based on selected energy modes
void sleep_block(const SLEEP_EnergyMode_t EM_s);


#endif /* SRC_ENERGY_MODES_H_ */
