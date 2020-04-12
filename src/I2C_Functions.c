/*
 * I2C_Functions.c
 *
 *  Created on: Feb 6, 2020
 *      Author: abhijeet
 */

#include"I2C_Functions.h"
#include"em_gpio.h"
#include"clock_init.h"

// Function to turn on I2C gpio pins


void load_on(void)
{
	GPIO_PinModeSet(gpioPortD,15,gpioModePushPull,true); // Enable Sensor pin PD15
}


// Function to turn off I2C gpio pins
void load_off(void)
{
	GPIO_PinOutClear(gpioPortD,15);
	GPIO_PinOutClear(gpioPortC,10);
	GPIO_PinOutClear(gpioPortC,11);
}

// Function to initialise letimer
void i2c_temp_init(void)
{
	I2CSPM_Init_TypeDef I2C_init = {
	I2C0,                       /* Use I2C instance 0 */
	gpioPortC,                  /* SCL port */
	10,                         /* SCL pin */
	gpioPortC,                  /* SDA port */
	11,                         /* SDA pin */
	14,                         /* Location of SCL */
	16,                         /* Location of SDA */
	0,                          /* Use currently configured reference clock */
	I2C_FREQ_STANDARD_MAX,      /* Set to standard rate  */
	i2cClockHLRStandard,        /* Set to use 4:4 low/high duty cycle */
	};
	load_on();
	I2CSPM_Init(&I2C_init); // Initialise I2C to sensor

}
// Function to write to sensor
void i2c_write(uint16_t pre_scalar)
{
	// Transfer write buffer
	uint8_t write_buffer[1] = {0xF3}; // Measure temperature, No hold master mode
	uint16_t w_length = (sizeof(write_buffer)/sizeof(uint8_t));

	I2C_TransferSeq_TypeDef i2c_wr;
	i2c_wr.addr = sensor_addr;
	i2c_wr.flags = I2C_FLAG_WRITE;
	i2c_wr.buf[0].data=write_buffer;
	i2c_wr.buf[0].len=w_length;

	int8_t write_val = 0;
	//i2c_temp_init(pre_scalar);

	write_val = I2CSPM_Transfer(I2C0,&i2c_wr);

	if(write_val != 0)
	{
		LOG_WARN("ERROR: %d",write_val);
	}
	else
	{
		LOG_INFO(" %d\n\r",write_val);
	}
	//timerWaitUs(count_cvt(pre_scalar,5));
	// Wait for acknowledgement
}
// Function to read data from sensor

void i2c_read(I2C_TransferSeq_TypeDef i2c_r)
{
	int8_t read_val = 0;
	read_val=I2CSPM_Transfer(I2C0,&i2c_r);
	if(read_val != 0)
	{
		LOG_WARN("ERROR: %d",read_val);
	}
	else
	{
		LOG_INFO(" %d \n\r",read_val);
	}
}

// Function convert 16bit temperature data to celsius
float temp_conv(void)
{
	float tempc;
	int16_t data = (read_buffer[0]<<8) + (read_buffer[1]);
	tempc = (float)((175.72*(data)) / 65536 )-46.85;
	return tempc;
}

uint8_t i2c_NB_write(void)
{

	uint8_t flag = 0;
	flag = I2C_TransferInit(I2C0,&i2c_wr);
	return flag;
}

I2C_TransferReturn_TypeDef i2c_NB_read(void)
{

	I2C_TransferReturn_TypeDef flag;
	flag = I2C_TransferInit(I2C0,&i2c_re);
	return flag;
}











