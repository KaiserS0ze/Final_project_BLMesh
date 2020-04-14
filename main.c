/***************************************************************************//**
 * @file
 * @brief Silicon Labs Bluetooth mesh light example
 * This example implements a Bluetooth mesh light node.
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
//Abhijeet Dutt Srivastava
// Reference: BT MESH LIGHT MESH EXAMPLE, BT MESH SWITCH EXAMPLE

/* Board headers */
#include "init_mcu.h"
#include "init_board.h"
#include "init_app.h"
#include "board_features.h"
#include "retargetserial.h"

/* Bluetooth stack headers */
#include "native_gecko.h"
#include "gatt_db.h"
#include <gecko_configuration.h>
#include <mesh_sizes.h>

/* Libraries containing default Gecko configuration values */
#include <em_gpio.h>

/* Coex header */
#include "coexistence-ble.h"

/* Device initialization header */
#include "hal-config.h"

/* Application code */
#include "app.h"

#if defined(HAL_CONFIG)
#include "bsphalconfig.h"
#else
#include "bspconfig.h"
#endif

#include "src/ble_mesh_device_type.h"

/***********************************************************************************************//**
 * @addtogroup Application
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup app
 * @{
 **************************************************************************************************/

bool mesh_bgapi_listener(struct gecko_cmd_packet *evt);

/// Maximum number of simultaneous Bluetooth connections
#define MAX_CONNECTIONS 2

/// Heap for Bluetooth stack
uint8_t bluetooth_stack_heap[DEFAULT_BLUETOOTH_HEAP(MAX_CONNECTIONS) + BTMESH_HEAP_SIZE + 1760];

/// Bluetooth advertisement set configuration
///
/// At minimum the following is required:
/// * One advertisement set for Bluetooth LE stack (handle number 0)
/// * One advertisement set for Mesh data (handle number 1)
/// * One advertisement set for Mesh unprovisioned beacons (handle number 2)
/// * One advertisement set for Mesh unprovisioned URI (handle number 3)
/// * N advertisement sets for Mesh GATT service advertisements
/// (one for each network key, handle numbers 4 .. N+3)
///
#define MAX_ADVERTISERS (4 + MESH_CFG_MAX_NETKEYS)

/// Priorities for bluetooth link layer operations
static gecko_bluetooth_ll_priorities linklayer_priorities = GECKO_BLUETOOTH_PRIORITIES_DEFAULT;

/// Bluetooth stack configuration
const gecko_configuration_t config =
{
  .bluetooth.max_connections = MAX_CONNECTIONS,
  .bluetooth.max_advertisers = MAX_ADVERTISERS,
  .bluetooth.heap = bluetooth_stack_heap,
  .bluetooth.heap_size = sizeof(bluetooth_stack_heap) - BTMESH_HEAP_SIZE,
#if defined(FEATURE_LFXO)
  .bluetooth.sleep_clock_accuracy = 100, // ppm
#elif defined(PLFRCO_PRESENT) || defined(LFRCO_PRESENT)
  .bluetooth.sleep_clock_accuracy = 500, // ppm
#endif
  .bluetooth.linklayer_priorities = &linklayer_priorities,
  .gattdb = &bg_gattdb_data,
  .btmesh_heap_size = BTMESH_HEAP_SIZE,
  .pa.config_enable = 1, // Set this to be a valid PA config
#if defined(FEATURE_PA_INPUT_FROM_VBAT)
  .pa.input = GECKO_RADIO_PA_INPUT_VBAT, // Configure PA input to VBAT
#else
  .pa.input = GECKO_RADIO_PA_INPUT_DCDC,
#endif // defined(FEATURE_PA_INPUT_FROM_VBAT)
  .max_timers = 16,
  .rf.flags = GECKO_RF_CONFIG_ANTENNA,   // Enable antenna configuration.
  .rf.antenna = GECKO_RF_ANTENNA,   // Select antenna path!
};


/*******************************************************************************************************
 * Main function.
 * Instructions to proceed for the mesh assignment.
 * 1. Add the gpio enable display function in gpio.c & gpio.h from Assignment 6.
 * 2. Complete displayUpdate() function in display.c similar to instructions in Assignment 6.
 * 3. Add your logic for loggerGetTimestamp() function in log.c from assignment 4.
 * 4. You can leverage your assignment files for timers, cmu etc. for this assignment.
 *
 * After completing above steps check for its functionality and proceed to mesh implementation.
 * 1. Use compile time switch in ble_mesh_device_type.h file to switch between publisher and subscriber.
 * 2. Add appropriate initializations in main before while loop.
 * 3. Then proceed to app.c for further instructions.
 *******************************************************************************************************/

// Custom libraries from previous assignments
#include "em_core.h"
#include "src/timer_module.h"
#include "src/clock_init.h"
#include "src/energy_modes.h"
#include "sleep.h"
#include "src/gpio.h"
#include "src/log.h"
#include "src/display.h"
#include "src/I2C_Functions.h"
#include "gpiointerrupt.h"
//#include "src/state_machine.h"

//Global Variables
uint16_t pre_scalar;
const SLEEP_EnergyMode_t EM_Select = sleepEM3;
uint16_t count;
uint32_t secs = 0;
uint8_t read_state = 0;
uint8_t write_state = 0;
bool BL_Flag = 0;
volatile uint8_t button_press = 0;
uint8_t signal;
volatile uint8_t Gpio_flag;

void ButtonHandler()
{
	gecko_external_signal(gecko_evt_system_external_signal_id);
	if(Gpio_flag == 0 )
	{
		Gpio_flag =1;
	}
	else if(Gpio_flag == 1)
	{
		Gpio_flag =0;
	}
	GPIO_IntClear(0x40);
}

int main(void)
{
  // Initialize device
  initMcu();
  // Initialize board
  initBoard();
  // Initialize application
  initApp();
  initVcomEnable();

  // Minimize advertisement latency by allowing the advertiser to always
  // interrupt the scanner.
  linklayer_priorities.scan_max = linklayer_priorities.adv_min + 1;

  gecko_stack_init(&config);

  //Initialisation for timer,display,gpio
    gpioInit();

    pre_scalar = clock_select(EM_Select,PERIOD);

    Lfa_Letimer0_enable();

    count=count_cvt(pre_scalar,PERIOD,EM_Select);

    letimer_init(count);

    letimer_interrupt_setting();
    sleep_block(EM_Select);
    logFlush();
    displayInit();
    logInit();

    GPIO_ExtIntConfig(button_port,button_pin,6,true,true,true);
    GPIOINT_Init();
    GPIOINT_CallbackRegister(button_pin,ButtonHandler);
    GPIO_IntEnable(1<<button_pin);

  //Initialisation for timer,display,gpio

  // Initialize the bgapi classes
  if( DeviceUsesClientModel() ){
	  gecko_bgapi_classes_init_client_lpn();
  }
  else {
	  gecko_bgapi_classes_init_server_friend();
  }

  // Initialize coexistence interface. Parameters are taken from HAL config.
  gecko_initCoexHAL();

  while (1) {

	struct gecko_cmd_packet *evt = gecko_wait_event();
    bool pass = mesh_bgapi_listener(evt);
    if (pass) {
      handle_ecen5823_gecko_event(BGLIB_MSG_ID(evt->header), evt);
    }

  }
}

void LETIMER0_IRQHandler(void)
{
	if(LETIMER0->IF & LETIMER_IFC_COMP0)
	{
		LETIMER_CounterSet(LETIMER0,65535);
		BL_Flag = 1;

		gecko_external_signal(gecko_evt_system_external_signal_id);
		secs = secs + 3000;
	}
	if(LETIMER0->IF & LETIMER_IFC_COMP1)
	{
		gecko_external_signal(gecko_evt_system_awake_id);
	}
	LETIMER_IntClear(LETIMER0,0x07);

}
