/*
 * state_machine.c
 *
 *  Created on: Feb 13, 2020
 *      Author: abhijeet
*/

#include "state_machine.h"

void I2CSM_Init(signal_t signal_s)
{
	if(events == eStart)
	{
		LOG_INFO("I2C_Init State\n\r");
		i2c_temp_init();
		SLEEP_SleepBlockBegin(sleepEM2);
		events = eWaiting80;
		states = sSleep;
		// handle write in IRQ based on event_s
	}
}

void sleep(signal_t signal_s, uint16_t pre_scalar)
{
	if(events == eWaiting80)
	{
			LOG_INFO("Sleep State eWaiting80\n\r");

			//SLEEP_SleepBlockBegin(sleepEM2);
			NB_Timer(80);
			struct gecko_cmd_packet* evt;
			evt = gecko_wait_event();
			gecko_ecen5823_update(evt);
			//SLEEP_SleepBlockEnd(sleepEM2);

			events = eStart_Write;
			states = sI2C_Write;
	}
	else if(events == eWaiting10)
	{
			LOG_INFO("Sleep State eWaiting10\n\r");

			//SLEEP_SleepBlockBegin(sleepEM2);
			NB_Timer(10);
			struct gecko_cmd_packet* evt;
			evt = gecko_wait_event();
			gecko_ecen5823_update(evt);
			//SLEEP_SleepBlockEnd(sleepEM2);

			states = sI2C_Read;
			events = eStart_Read;
	}
	else
	{
		states = sDefault;
		events = erandom;
	}
}

void I2C_write(signal_t signal_s)
{
	if(events == eStart_Write)
	{
		LOG_INFO("I2C Write State\n\r");

		i2c_NB_write();
		NVIC_EnableIRQ(I2C0_IRQn);

		if(write_state == 1)
		{
			//I2C_IntClear(I2C0,0x7E0);
			states = sSleep;
			events = eWaiting10;
		}
	}
}

void I2C_read(signal_t signal_s)
{

	if(events == eStart_Read)
	{

		read_check = 1;

		LOG_INFO("I2C Read State\n\r");

		NVIC_DisableIRQ(I2C0_IRQn);
		i2c_NB_read();
		NVIC_EnableIRQ(I2C0_IRQn);
		if(read_state == 1)
		{
			read_state = 0;
			states = sDisplay_data;
			events = eDisplay_Data;
		}
	}
}

void flip(char* str, int len)
{
    int i = 0, j = len - 1, temp;
    while (i < j) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}

int integer_string(int x, char str[], int d)
{
    int i = 0;
    while (x) {
        str[i++] = (x % 10) + '0';
        x = x / 10;
    }

    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';

    flip(str, i);
    str[i] = '\0';
    return i;
}

void float_str(float n, char* res, int afterpoint)
{
    // Extract integer part
    int ipart = (int)n;

    // Extract floating part
    float fpart = n - (float)ipart;

    // convert integer part to string
    int i = integer_string(ipart, res, 0);

    // check for display option after point
    if (afterpoint != 0) {
        res[i] = '.'; // add dot

        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter
        // is needed to handle cases like 233.007
        fpart = fpart * pow(10, afterpoint);

        integer_string((int)fpart, res + i + 1, afterpoint);
    }
}

void display_data(signal_t signal_s)
{
	if(events == eDisplay_Data)
	{
		const char *server = "server";
		char buffer[5];

		//timerWaitUs(count_cvt(pre_scalar,10));
		LOG_INFO("DISPLAY DATA STATE\n\r");
		read_check = 1;
		//function to display temperature
		//float cels;
		gecko_cmd_system_get_bt_address();
		cels = temp_conv();
		float_str(cels,buffer,4);
		LOG_INFO(" Temp %f\n\r",cels);
		//displayPrintf(DISPLAY_ROW_BTADDR,);
		displayPrintf(DISPLAY_ROW_NAME,server);
		displayPrintf(DISPLAY_ROW_TEMPVALUE,buffer);
		states = sStop;
		events = eStop;
	}
}

void stop_state(signal_t signal_s)
{
	LOG_INFO("STOP STATE\n\r");

	if(events == eStop)
	{
		BL_Flag = 0;
		//load_off();
		SLEEP_SleepBlockEnd(sleepEM2);
	}
	else
	{
		// log random
		//LOG_INFO("STOP STATE- UNEXPECTED\n\r");
	}
}

void random_state(signal_t signal_s)
{
	//LOG_INFO("RANDOM STATE\n\r");

	if(events == erandom)
	{
		// log error has occured in state machine
		//LOG_INFO("STOP STATE- erandom\n\r");
//		events = eStart;
//		states = sI2C_Init;
	}
}

void state_machine(signal_t signal_m,uint16_t pre_scalar)
{
	switch(states)
	{
	case sI2C_Init:
		// I2CSPM_Function
		I2CSM_Init(signal_m);
		break;
	case sSleep:
		//EM1 Sleep
		sleep(signal_m,pre_scalar);
		break;
	case sI2C_Write:
		// I2C_TransferInit
		I2C_write(signal_m);
		break;
	case sI2C_Read:
		// I2C_TransferInit
		I2C_read(signal_m);
		break;
	case sDisplay_data:
		// display_data
		display_data(signal_m);
		break;
	case sStop:
		// end case
		stop_state(signal_m);
		break;
	case sDefault:
		// unexpected case
		random_state(signal_m);
		break;
	}
}

