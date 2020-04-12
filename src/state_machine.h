/*
 * state_machine.h
 *
 *  Created on: Feb 13, 2020
 *      Author: abhijeet
*/

#include "I2C_Functions.h"
#include "clock_init.h"
#define INCLUDE_LOG_DEBUG 1
#include "log.h"
#include "em_core.h"
#include "em_emu.h"
#include "native_gecko.h"
#include "bluetooth.h"
#include "display.h"
#include <math.h>

#define SCHEDULER_SUPPORTS_DISPLAY_UPDATE_EVENT 1

typedef enum {
eStart,
eStop,
eStart_Write,
eIn_Progress,
eStart_Read,
eError,
eWaiting80,
eWaiting10,
eDisplay_Data,
erandom,
}events_t;

typedef enum{
sI2C_Init,
sSleep,
sI2C_Write,
sI2C_Read,
sDisplay_data,
sDefault,
sStop,
}state_t;


typedef enum{
sigWrite,
sigRead,
}signal_t;


extern state_t states;
extern events_t events;
extern signal_t signal;
extern uint8_t write_check;
extern uint8_t read_check;
extern uint8_t read_state;
extern uint8_t write_state;
extern uint16_t pre_scalar;
extern float cels;
extern bool BL_Flag;

#ifndef SRC_STATE_MACHINE_H_
#define SRC_STATE_MACHINE_H_

// State to initialise I2C
void I2CSM_Init(signal_t signal_s);
// State in which delays are set
void sleep(signal_t signal_s, uint16_t pre_scalar);
// State in which I2C wites to Si7021
void I2C_write(signal_t signal_s);
// State in which RX data is received
void I2C_read(signal_t signal_s);
// State in which temperature is displayed
void display_data(signal_t signal_s);
// State in which EM3 is started and state machine ends
void stop_state(signal_t signal_s);
// State to handle random events
void random_state(signal_t signal_s);
// State machine handler
void state_machine(signal_t signal_m,uint16_t pre_scalar);

// Reference: https://www.geeksforgeeks.org/convert-floating-point-number-string/
// Functions to convert float to string
void flip(char* str, int len);

int integer_string(int x, char str[], int d);

void float_str(float n, char* res, int afterpoint);

#endif  SRC_STATE_MACHINE_H_
