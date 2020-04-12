/*
 * bluetooth.h
 *
 *  Created on: Feb 22, 2020
 *      Author: abhijeet
 */


#include <stdint.h>

#include "native_gecko.h"
#include "gatt_db.h"
#include "display.h"
#include "ble_device_type.h"

/* Gecko configuration parameters (see gecko_configuration.h) */
#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS 2
#endif
extern volatile uint8_t button_press;
#if DEVICE_IS_BLE_SERVER
extern float cels;
uint8_t states;
uint8_t events;

#else

#include "bg_types.h"
#ifdef FEATURE_BOARD_DETECTED
#if defined(HAL_CONFIG)
#include "bsphalconfig.h"
#else
#include "bspconfig.h"
#endif
#else
#error This sample app only works with a Silicon Labs Board
#endif

#include "stdio.h"
#include "retargetserial.h"
#include "gpiointerrupt.h"

// Counter of active connections
uint8_t activeConnectionsNum;
#endif

#ifndef SRC_BLUETOOTH_H_
#define SRC_BLUETOOTH_H_

// State machine for server and client
void gecko_ecen5823_update(struct gecko_cmd_packet* evt);

#if DEVICE_IS_BLE_SERVER == 0
void initProperties(void);
uint8_t findServiceInAdvertisement(uint8_t *data, uint8_t len);
uint8_t findServiceInButtonAdvertisement(uint8_t *data, uint8_t len);
uint8_t findIndexByConnectionHandle(uint8_t connection);
void addConnection(uint8_t connection, uint16_t address);
void removeConnection(uint8_t connection);
#endif

#endif /* SRC_BLUETOOTH_H_ */
