/*
 * I2C_Functions.h
 *
 *  Created on: Feb 6, 2020
 *      Author: abhijeet
 */

#include "i2cspm.h"
#define INCLUDE_LOG_DEBUG 1
#include "log.h"
#include "em_i2c.h"

extern uint8_t read_buffer[2];
extern uint8_t write_buffer[1];

extern I2C_TransferSeq_TypeDef i2c_re;
extern I2C_TransferSeq_TypeDef i2c_wr;

#ifndef SRC_I2C_FUNCTIONS_H_
#define SRC_I2C_FUNCTIONS_H_
//Sensor Address
#define sensor_addr (0x40<<1)

// Function to initialise I2C peripheral
void i2c_temp_init(void);
// Function to write to sensor
void i2c_write(uint16_t pre_scalar);
// Function to read data from sensor
void i2c_read(I2C_TransferSeq_TypeDef i2c_r);
// Function convert 16bit temperature data to celsius
float temp_conv(void);
// Function to turn on I2C gpio pins
void load_on(void);
// Function to turn off I2C gpio pins
void load_off(void);

uint8_t i2c_NB_write(void);
I2C_TransferReturn_TypeDef i2c_NB_read(void);

#endif /* SRC_I2C_FUNCTIONS_H_ */
