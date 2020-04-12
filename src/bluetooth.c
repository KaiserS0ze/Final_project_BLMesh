/*
 * bluetooth.c
 *
 *  Created on: Feb 22, 2020
 *      Author: abhijeet
 */

#include "bluetooth.h"
//#include "native_gecko.h"
//#include "gatt_db.h"
#include "app/bluetooth/common/util/infrastructure.h"
#include "state_machine.h"
#include "log.h"
#include "src/gecko_ble_errors.h"
#include "gpio.h"
//#include "ble_device_type.h"

typedef enum {
  scanning,
  opening,
  discoverServices,
  discoverCharacteristics,
  enableIndication,
  running
} ConnState;

typedef struct {
  uint8_t  connectionHandle;
  int8_t   rssi;
  uint16_t serverAddress;
  uint32_t thermometerServiceHandle;
  uint16_t thermometerCharacteristicHandle;
  uint32_t temperature;
  uint32_t buttonServiceHandle;
  uint16_t buttonCharacteristicHandle;
  uint32_t button;
} ConnProperties;

// Client Side
// connection parameters
#define CONN_INTERVAL_MIN             80   //100ms
#define CONN_INTERVAL_MAX             80   //100ms
#define CONN_SLAVE_LATENCY            0    //no latency
#define CONN_TIMEOUT                  100  //1000ms

#define SCAN_INTERVAL                 16   //10ms
#define SCAN_WINDOW                   16   //10ms
#define SCAN_PASSIVE                  0

#define TEMP_INVALID                  (uint32_t)0xFFFFFFFFu
#define RSSI_INVALID                  (int8_t)127
#define CONNECTION_HANDLE_INVALID     (uint8_t)0xFFu
#define SERVICE_HANDLE_INVALID        (uint32_t)0xFFFFFFFFu
#define CHARACTERISTIC_HANDLE_INVALID (uint16_t)0xFFFFu
#define TABLE_INDEX_INVALID           (uint8_t)0xFFu

#define EXT_SIGNAL_PRINT_RESULTS      0x01

// Gecko configuration parameters (see gecko_configuration.h)

// Flag for indicating DFU Reset must be performed
uint8_t bootToDfu = 0;
// Array for holding properties of multiple (parallel) connections
ConnProperties connProperties[MAX_CONNECTIONS];

// State of the connection under establishment
ConnState connState;
// Health Thermometer service UUID defined by Bluetooth SIG
const uint8_t thermoService[2] = { 0x09, 0x18 };
// Temperature Measurement characteristic UUID defined by Bluetooth SIG
const uint8_t thermoChar[2] = { 0x1c, 0x2a };

const uint8_t ButtonService[16] = {0x89,0x62,0x13,0x2d,0x2a,0x65,0xec,0x87,0x3e,0x43,0xc8,0x38,0x01,0x00,0x00,0x00};

const uint8_t ButtonCharacteristic[16] = {0x89,0x62,0x13,0x2d,0x2a,0x65,0xec,0x87,0x3e,0x43,0xc8,0x38,0x02,0x00,0x00,0x00};

void gecko_ecen5823_update(struct gecko_cmd_packet* evt)
{
	uint8_t button_v = 0;
    uint8_t button_state = 0;
    uint8_t button_ar[1];

#if DEVICE_IS_BLE_SERVER
	uint8_t buffer[5];
	uint32_t temp;
	uint8_t *ptr = buffer;
	int connection;
	/* Flag for indicating DFU Reset must be performed */
	uint8_t boot_to_dfu = 0;
	char * connected = "Connected";
	char * advertising = "Advertising";

#else
	uint8_t tableIndex;
	uint8_t* charValue;
	bool printHeader = true;
	int8_t i;
	uint8_t TempFlg = 0;
	uint8_t BtnFlg = 0;
	uint8_t Service = 0;
	uint16_t addrValue;
	bd_addr ADDRESS = SERVER_BT_ADDRESS;
	const char *client = "client";
	displayPrintf(DISPLAY_ROW_NAME,client);
	char server_a[10];
#endif

	switch (BGLIB_MSG_ID(evt->header)) {
	#if DEVICE_IS_BLE_SERVER
      /* This boot event is generated when the system boots up after reset.
       * Do not call any stack commands before receiving the boot event.
       * Here the system is set to start advertising immediately after boot procedure. */
	   case gecko_evt_system_boot_id:

		   // Delete Bondings
		   gecko_cmd_sm_delete_bondings();
		   // Configure security reqs and I/O capabilities of system
		   gecko_cmd_sm_configure(0x05,sm_io_capability_displayyesno);
		   // Bonding allowed by setting Bit 1 in configure
		   gecko_cmd_sm_set_bondable_mode(1);

    	/* Set advertising parameters. 100ms advertisement interval.
         * The first two parameters are minimum and maximum advertising interval, both in
         * units of (milliseconds * 1.6). */

		   BTSTACK_CHECK_RESPONSE(gecko_cmd_le_gap_set_advertise_timing(0, 400, 400, 0, 0));
        /* Start general advertising and enable connections. */
           BTSTACK_CHECK_RESPONSE(gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable));

        displayPrintf(DISPLAY_ROW_CONNECTION,"Advertising");
        bd_addr addr=gecko_cmd_system_get_bt_address()->address;
        	        char buff[15];
        	        sprintf(buff,"%x:%x:%x:%x:%x:%x",addr.addr[5],addr.addr[4],addr.addr[3],addr.addr[2],addr.addr[1], addr.addr[0]);
        	        LOG_INFO("\n\r%s",buff);
        	        displayPrintf(DISPLAY_ROW_BTADDR,buff);

        LOG_INFO("\n]rAdvertising started");
        break;

        // event to display passkey

        /*case gecko_evt_sm_passkey_display_id:
        LOG_INFO("---->Passkey Display\n<----");
        displayPrintf(DISPLAY_ROW_PASSKEY,"PassKey %d",evt->data.evt_sm_passkey_display.passkey);
        displayPrintf(DISPLAY_ROW_ACTION,"PassKey Confirm");*/
        //break;

        // Ask the user to confirm the passkey --> Numeric Comparison
        case gecko_evt_sm_confirm_passkey_id:
        	LOG_INFO("---->Confirm Passkey<----\n");
        	// Confirm passkey ---> hardcoded to 1 for now
        	displayPrintf(DISPLAY_ROW_PASSKEY,"PassKey %d",evt->data.evt_sm_confirm_passkey.passkey);
        	displayPrintf(DISPLAY_ROW_ACTION,"Confirm with PB0");
        	// This logic because -- PB0 is pulled up
        	if(button_press == 1)
        	{
        		button_v = 0;
        	}
        	else
        	{
        		button_v = 1;
        	}
        	LOG_DEBUG("\n\n---->button value = %d<-----\n\n",button_v);
        	gecko_cmd_sm_passkey_confirm(evt->data.evt_sm_confirm_passkey.connection,button_v);
        	break;

        // Bonding -- Pairing successful
        case gecko_evt_sm_bonded_id:
          LOG_INFO("---->Passkey Bonded<----\n");
          displayPrintf(DISPLAY_ROW_ACTION,"Bonded");
        break;

        // confirm bonding is correct --> because Bit 2 in configure is 1 // otherwise not needed
	    case gecko_evt_sm_confirm_bonding_id:
	    //Accept bonding request
	    LOG_INFO("---->Confirm Bonding<----\n");
	    BTSTACK_CHECK_RESPONSE(gecko_cmd_sm_set_passkey(000456));
	    // confirm or reject the bonding request
	    gecko_cmd_sm_bonding_confirm(evt->data.evt_sm_confirm_bonding.connection,1);

	    break;

        // Bonding -- Paring Failed
	    case gecko_evt_sm_bonding_failed_id:
	    	LOG_INFO("---->Bonding Failed<----\n");
	    break;

	    case gecko_evt_le_connection_opened_id:
	    displayPrintf(DISPLAY_ROW_CONNECTION,connected);
    	LOG_INFO("\n\rOpen");
        NVIC_EnableIRQ(LETIMER0_IRQn);
        connection = evt->data.evt_le_connection_opened.connection;
        //setting parameters for the connection case
        BTSTACK_CHECK_RESPONSE(gecko_cmd_le_connection_set_parameters(connection,0x003c,0x003c,3,600));
        BTSTACK_CHECK_RESPONSE(gecko_cmd_le_connection_get_rssi(connection));
        break;

          /* This event is generated when a connected client has either
          * 1) changed a Characteristic Client Configuration, meaning that they have enabled
          * or disabled Notifications or Indications, or
          * 2) sent a confirmation upon a successful reception of the indication. */
      case gecko_evt_gatt_server_characteristic_status_id:
    	  /* Check that the characteristic in question is temperature - its ID is defined
         * in gatt.xml as "temperature_measurement". Also check that status_flags = 1, meaning that
         * the characteristic client configuration was changed (notifications or indications
         * enabled or disabled). */
//Commented in assignment 10
    	  /*if ((evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_temperature_measurement)
            && (evt->data.evt_gatt_server_characteristic_status.status_flags == 0x01)) {
          if (evt->data.evt_gatt_server_characteristic_status.client_config_flags == 0x02) {
             Indications have been turned ON - start the repeating timer. The 1st parameter '32768'
             * tells the timer to run for 1 second (32.768 kHz oscillator), the 2nd parameter is
             * the timer handle and the 3rd parameter '0' tells the timer to repeat continuously until
             * stopped manually.
        	  BTSTACK_CHECK_RESPONSE(gecko_cmd_hardware_set_soft_timer(32768, 0, 0));
          }
          else if (evt->data.evt_gatt_server_characteristic_status.client_config_flags == 0x00) {
             Indications have been turned OFF - stop the timer.
        	  BTSTACK_CHECK_RESPONSE(gecko_cmd_hardware_set_soft_timer(0, 0, 0));
          }
        }*/
//Commented in assignment 10
        /*else if((evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_button_state)
        		&& (evt->data.evt_gatt_server_characteristic_status.status_flags == 0x01) )
        {
        	//LOG_INFO("\n\n ---> BUTTON <----\n\n");
        	button_state = GPIO_PinInGet(button_port,button_pin);
        	if(button_state == 1)
        	  	    {
        	  	     button_ar[0] = 0;
        	        }
        	  	     else
        	  	    {
        	  	     button_ar[0] = 1;
        	  	    }
        	  	    gecko_cmd_gatt_server_write_attribute_value(gattdb_button_state,0,sizeof(uint8_t),button_ar);
        	  	    BTSTACK_CHECK_RESPONSE(gecko_cmd_gatt_server_send_characteristic_notification(0xFF,gattdb_button_state,1,button_ar));
        }*/
        break;

      /* This event is generated when the software timer has ticked. In this example the temperature
       * is read after every 1 second and then the indication of that is sent to the listening client. */

      case gecko_evt_hardware_soft_timer_id:
    	  LOG_INFO("\n\rCharacter");
    	  UINT8_TO_BITSTREAM(ptr, 0);

    	  temp = FLT_TO_UINT32(cels*1000, -3);

    	  /* Convert temperature to bitstream and place it in the HTM temperature data buffer (htmTempBuffer) */

    	  UINT32_TO_BITSTREAM(ptr, temp);
    	  //Commented in assignment 10
    	  //BTSTACK_CHECK_RESPONSE(gecko_cmd_gatt_server_send_characteristic_notification(0xFF,gattdb_temperature_measurement,5,buffer));
    	  break;

      case gecko_evt_le_connection_closed_id:
    	  LOG_INFO("\n\rClosed");
    	  displayPrintf(DISPLAY_ROW_PASSKEY,"");
    	  displayPrintf(DISPLAY_ROW_ACTION,"");
    	  displayPrintf(DISPLAY_ROW_CONNECTION,"Advertising");
    	  displayPrintf(DISPLAY_ROW_TEMPVALUE,"");

        /* Check if need to boot to dfu mode */
        if (boot_to_dfu) {
          /* Enter to DFU OTA mode */
        	gecko_cmd_system_reset(2);
        } else {
          /* Stop timer in case client disconnected before indications were turned off */
        	BTSTACK_CHECK_RESPONSE(gecko_cmd_hardware_set_soft_timer(0, 0, 0));
          /* Restart advertising after client has disconnected */
        	BTSTACK_CHECK_RESPONSE(gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable));
        }
        break;
      /* Events related to OTA upgrading
         ----------------------------------------------------------------------------- */

      /* Checks if the user-type OTA Control Characteristic was written.
       * If written, boots the device into Device Firmware Upgrade (DFU) mode. */
      case gecko_evt_gatt_server_user_write_request_id:
    	  LOG_INFO("\nWrite");
    	  if (evt->data.evt_gatt_server_user_write_request.characteristic == gattdb_ota_control) {
          /* Set flag to enter to OTA mode */
          boot_to_dfu = 1;
          /* Send response to Write Request */
          BTSTACK_CHECK_RESPONSE(gecko_cmd_gatt_server_send_user_write_response(
            evt->data.evt_gatt_server_user_write_request.connection,
            gattdb_ota_control,
            bg_err_success));

          /* Close connection to enter to DFU OTA mode */
          BTSTACK_CHECK_RESPONSE(gecko_cmd_le_connection_close(evt->data.evt_gatt_server_user_write_request.connection));
        }
        break;
          //TX power
        case gecko_evt_le_connection_rssi_id:
        	BTSTACK_CHECK_RESPONSE(gecko_cmd_system_halt(1));
             if(evt->data.evt_le_connection_rssi.rssi>-35)
             {
                //command to set TX power
               gecko_cmd_system_set_tx_power(-250);
             }
             else if(evt->data.evt_le_connection_rssi.rssi<=-35  && evt->data.evt_le_connection_rssi.rssi>=-45 )
             {
               gecko_cmd_system_set_tx_power(-200);
             }
               else if(evt->data.evt_le_connection_rssi.rssi<=-45 && evt->data.evt_le_connection_rssi.rssi>=-55)
             {
               gecko_cmd_system_set_tx_power(-150);
             }
               else if(evt->data.evt_le_connection_rssi.rssi<=-55 && evt->data.evt_le_connection_rssi.rssi>=-65)
             {
               gecko_cmd_system_set_tx_power(-50);
             }
               else if(evt->data.evt_le_connection_rssi.rssi<=-65 && evt->data.evt_le_connection_rssi.rssi>=-75)
             {
               gecko_cmd_system_set_tx_power(0);
             }
               else if(evt->data.evt_le_connection_rssi.rssi<=-75 && evt->data.evt_le_connection_rssi.rssi>=-85)
             {
               gecko_cmd_system_set_tx_power(50);
             }
               else if(evt->data.evt_le_connection_rssi.rssi<=-85)
             {
                gecko_cmd_system_set_tx_power(100);
             }
               else{
                gecko_cmd_system_set_tx_power(0);
             }
             BTSTACK_CHECK_RESPONSE(gecko_cmd_system_halt(0));
               break;

      case gecko_evt_system_external_signal_id:
          break;

      case gecko_evt_system_awake_id:
    	  break;

      default:
        break;
	 #else
        // This boot event is generated when the system boots up after reset
      case gecko_evt_system_boot_id:
               printf("\r\nBLE Central started for the client\r\n");
               LOG_INFO("\n\r      CLIENT              ");
               // Deleting the bondings
      		  //gecko_cmd_sm_delete_bondings();

      		  // Security configurations and setting IO to display yes/no config
      		  gecko_cmd_sm_configure(0x05,sm_io_capability_displayyesno);

      		  // Allow bonding
      		  gecko_cmd_sm_set_bondable_mode(1);

               // Set passive scanning on 1Mb PHY
               BTSTACK_CHECK_RESPONSE(gecko_cmd_le_gap_set_discovery_type(le_gap_phy_1m, SCAN_PASSIVE));
               // Set scan interval and scan window
               BTSTACK_CHECK_RESPONSE(gecko_cmd_le_gap_set_discovery_timing(le_gap_phy_1m, SCAN_INTERVAL, SCAN_WINDOW));
               // Set the default connection parameters for subsequent connections
               BTSTACK_CHECK_RESPONSE(gecko_cmd_le_gap_set_conn_parameters(CONN_INTERVAL_MIN,
                                                    CONN_INTERVAL_MAX,
                                                    CONN_SLAVE_LATENCY,
                                                    CONN_TIMEOUT));
               // Start scanning - looking for thermometer devices
               BTSTACK_CHECK_RESPONSE(gecko_cmd_le_gap_start_discovery(le_gap_phy_1m, le_gap_discover_generic));
               displayPrintf(DISPLAY_ROW_CONNECTION,"Discovering");
               connState = scanning;
               displayPrintf(DISPLAY_ROW_NAME, BLE_DEVICE_TYPE_STRING);
               bd_addr adr=gecko_cmd_system_get_bt_address()->address;
      		 char tem[18];
      		 sprintf(tem,"%x:%x:%x:%x:%x:%x",adr.addr[5],adr.addr[4],adr.addr[3],adr.addr[2],adr.addr[1], adr.addr[0]);
      		 LOG_INFO("\n\r%s",tem);
      		 displayPrintf(DISPLAY_ROW_BTADDR,tem);
               break;


          // Ask the user to confirm the passkey --> Numeric Comparison
          case gecko_evt_sm_confirm_passkey_id:
          LOG_INFO("---->Confirm Passkey<----\n");
          // Confirm passkey ---> hardcoded to 1 for now
          displayPrintf(DISPLAY_ROW_PASSKEY,"PassKey %d",evt->data.evt_sm_confirm_passkey.passkey);
          displayPrintf(DISPLAY_ROW_ACTION,"Confirm with PB0");
          // This logic because -- PB0 is pulled up
          if(button_press == 1)
          {
               button_v = 0;
          }
          else
          {
               button_v = 1;
          }
          LOG_DEBUG("\n\n---->button value = %d<-----\n\n",button_v);
          gecko_cmd_sm_passkey_confirm(evt->data.evt_sm_confirm_passkey.connection,button_v);
          break;

          // Bonding -- Pairing successful
          case gecko_evt_sm_bonded_id:
          LOG_INFO("---->Passkey Bonded<----\n");
          displayPrintf(DISPLAY_ROW_ACTION,"Bonded");
          if(1 == BtnFlg)
          {
          	LOG_INFO("\n--------------------------Button service found-------------------------\n");
          	BTSTACK_CHECK_RESPONSE((gecko_cmd_gatt_discover_characteristics_by_uuid(evt->data.evt_gatt_procedure_completed.connection,connProperties[tableIndex].buttonServiceHandle,16,(const uint8_t*)ButtonCharacteristic)));
          }
          break;

          // confirm bonding is correct --> because Bit 2 in configure is 1 // otherwise not needed
          case gecko_evt_sm_confirm_bonding_id:
          //Accept bonding request
          LOG_INFO("---->Confirm Bonding<----\n");
          BTSTACK_CHECK_RESPONSE(gecko_cmd_sm_set_passkey(000456));
          // confirm or reject the bonding request
          gecko_cmd_sm_bonding_confirm(evt->data.evt_sm_confirm_bonding.connection,1);
          break;

          // Bonding -- Paring Failed
          case gecko_evt_sm_bonding_failed_id:
          LOG_INFO("---->Bonding Failed<----\n");
          break;

        // This event is generated when an advertisement packet or a scan response
        // is received from a slave
        case gecko_evt_le_gap_scan_response_id:
            // Parse advertisement packets
    //         if (evt->data.evt_le_gap_scan_response.packet_type == 0) {
               // If a thermometer advertisement is found...
               if (findServiceInAdvertisement(&(evt->data.evt_le_gap_scan_response.data.data[0]),
                                              evt->data.evt_le_gap_scan_response.data.len) != 0) {
                 // then stop scanning for a while
               BTSTACK_CHECK_RESPONSE(gecko_cmd_le_gap_end_procedure());
          	   BTSTACK_CHECK_RESPONSE(gecko_cmd_le_gap_connect(evt->data.evt_le_gap_scan_response.address,evt->data.evt_le_gap_scan_response.address_type,le_gap_phy_1m));

            	 Service = 1;
               }
               else if((findServiceInButtonAdvertisement(&(evt->data.evt_le_gap_scan_response.data.data[0]),
                         evt->data.evt_le_gap_scan_response.data.len) != 0))
               {
    			//LOG_INFOss("in button service");
    		   BTSTACK_CHECK_RESPONSE(gecko_cmd_le_gap_end_procedure());
         	   BTSTACK_CHECK_RESPONSE(gecko_cmd_le_gap_connect(evt->data.evt_le_gap_scan_response.address,evt->data.evt_le_gap_scan_response.address_type,le_gap_phy_1m));

    			}

               if(activeConnectionsNum < MAX_CONNECTIONS)
               {
                   connState = opening;
               }

    //         }
             break;

        // This event is generated when a new connection is established
        case gecko_evt_le_connection_opened_id:
          // Get last two bytes of sender address

          addrValue = ((ADDRESS.addr[1] << 8) + ADDRESS.addr[0]);
          /*addrValue = (uint16_t)(evt->data.evt_le_connection_opened.address.addr[1] << 8) \
                      + evt->data.evt_le_connection_opened.address.addr[0];*/
          // Add connection to the connection_properties array
          addConnection(evt->data.evt_le_connection_opened.connection, addrValue);

          // Increasing the security
          BTSTACK_CHECK_RESPONSE(gecko_cmd_sm_increase_security(evt->data.evt_le_connection_opened.connection));


          // Discover Health Thermometer service on the slave device
          //BTSTACK_CHECK_RESPONSE(gecko_cmd_gatt_discover_primary_services_by_uuid(evt->data.evt_le_connection_opened.connection,2,(const uint8_t*)thermoService));
          //discover primary services
          BTSTACK_CHECK_RESPONSE(gecko_cmd_gatt_discover_primary_services(evt->data.evt_le_connection_opened.connection));

          connState = discoverServices;
          break;

          // This event is generated when a new service is discovered
                case gecko_evt_gatt_service_id:
           		 tableIndex = findIndexByConnectionHandle(evt->data.evt_gatt_service.connection);
           		 if (tableIndex != TABLE_INDEX_INVALID) {

           		   // Save service handle for future reference
           		   // if((evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_temperature_measurement))
           			 if(!(memcmp(evt->data.evt_gatt_service.uuid.data,thermoService,2)))
           			 {
           				 connProperties[tableIndex].thermometerServiceHandle = evt->data.evt_gatt_service.service;
           				 LOG_INFO("\nEntered temp check in gatt service\n");
           				 Service = 0;
           				 TempFlg = 1;
           			 }
           			  else if(!(memcmp(evt->data.evt_gatt_service.uuid.data,ButtonService,16)))					{
           						connProperties[tableIndex].buttonServiceHandle = evt->data.evt_gatt_service.service;
           						LOG_INFO("\nEntered button check in gatt service\n");
           						Service = 1;
           						BtnFlg = 1;
           					}
           		 }
                  break;

                // This event is generated when a new characteristic is discovered
                case gecko_evt_gatt_characteristic_id:
                	tableIndex = findIndexByConnectionHandle(evt->data.evt_gatt_characteristic.connection);
                			 if (tableIndex != TABLE_INDEX_INVALID) {
                			   // Save characteristic handle for future reference
                					if(!(memcmp(evt->data.evt_gatt_characteristic.uuid.data,thermoChar,2)))			 {
                					 connProperties[tableIndex].thermometerCharacteristicHandle = evt->data.evt_gatt_characteristic.characteristic;
                					 LOG_INFO("\nEntered temp check in gatt temperature charac\n");
                				 }
                					if(!(memcmp(evt->data.evt_gatt_characteristic.uuid.data,ButtonCharacteristic,16)))			 {
                					 connProperties[tableIndex].buttonCharacteristicHandle = evt->data.evt_gatt_characteristic.characteristic;
                					 LOG_INFO("\nEntered temp check in gatt button service\n");
                				 }
                			 }
                			 break;

                // This event is generated for various procedure completions, e.g. when a
                // write procedure is completed, or service discovery is completed
                case gecko_evt_gatt_procedure_completed_id:
           		 tableIndex = findIndexByConnectionHandle(evt->data.evt_gatt_procedure_completed.connection);
           		 if (tableIndex == TABLE_INDEX_INVALID) {
           		   break;
           		 }
           		 // If service discovery finished
           		 if (connState == discoverServices \
           			 && connProperties[tableIndex].thermometerServiceHandle != SERVICE_HANDLE_INVALID && (TempFlg == 1) )
           		 {
           			 LOG_INFO("\nEntered temp procedure completed in gatt service\n");
           			 // Discover thermometer characteristic on the slave device
           			 BTSTACK_CHECK_RESPONSE(gecko_cmd_gatt_discover_characteristics_by_uuid(evt->data.evt_gatt_procedure_completed.connection,
           														   connProperties[tableIndex].thermometerServiceHandle,
           														   2,
           														   (const uint8_t*)thermoChar));
           			 connState = discoverCharacteristics;
           			 break;
           		 }
           			 else if (connState == discoverServices \
           								  && connProperties[tableIndex].buttonServiceHandle != SERVICE_HANDLE_INVALID && (BtnFlg == 1))
           			 {
           				 LOG_INFO("\nEntered temp procedure completed in gatt button\n");
           				// Discover thermometer characteristic on the slave device
           				 BTSTACK_CHECK_RESPONSE(gecko_cmd_gatt_discover_characteristics_by_uuid(evt->data.evt_gatt_procedure_completed.connection,
           																connProperties[tableIndex].buttonServiceHandle,
           																16,
           																(const uint8_t*)ButtonCharacteristic));

           				connState = discoverCharacteristics;
           		   break;
           			 }

           		 // If characteristic discovery finished
           		 if (connState == discoverCharacteristics \
           			 && connProperties[tableIndex].thermometerCharacteristicHandle != CHARACTERISTIC_HANDLE_INVALID && (TempFlg == 1) ) {
           		   // stop discovering
           			 BTSTACK_CHECK_RESPONSE(gecko_cmd_le_gap_end_procedure());
           			 Service = !Service;
           		   // enable indication
           			 BTSTACK_CHECK_RESPONSE(gecko_cmd_gatt_set_characteristic_notification(evt->data.evt_gatt_procedure_completed.connection,
           														  connProperties[tableIndex].thermometerCharacteristicHandle,
           														  gatt_indication));
           			 LOG_INFO("\nset charac notification for temperature\n");
           			 displayPrintf(DISPLAY_ROW_CONNECTION, "Handling Indications");
           								connState = enableIndication;
           								TempFlg = 0;
           								break;
           			 }
           			 else if(connState == discoverCharacteristics \
           					 && connProperties[tableIndex].buttonCharacteristicHandle != CHARACTERISTIC_HANDLE_INVALID && (BtnFlg == 1) )
           			 {

           				 Service = !Service;
           				 BTSTACK_CHECK_RESPONSE(gecko_cmd_gatt_set_characteristic_notification(evt->data.evt_gatt_procedure_completed.connection,
           																				  connProperties[tableIndex].buttonCharacteristicHandle,
           																				  gatt_indication));
           				 LOG_INFO("\nset charac notification for button\n");
           				 displayPrintf(DISPLAY_ROW_CONNECTION, "Handling Indications");
           									connState = enableIndication;
           									BtnFlg = 0;
           									break;
           			 }


           			 // If indication enable process finished
           			 if (connState == enableIndication) {
           			   // and we can connect to more devices
           			   if (activeConnectionsNum < MAX_CONNECTIONS) {
           				 // start scanning again to find new devices
           				   BTSTACK_CHECK_RESPONSE(gecko_cmd_le_gap_start_discovery(le_gap_phy_1m, le_gap_discover_generic));
           				 connState = scanning;
           			   } else {
           				 connState = running;
           			   }
           			   break;
           			 }
           			 break;

                // This event is generated when a connection is dropped
                case gecko_evt_le_connection_closed_id:
                  // Check if need to boot to dfu mode
                  if (bootToDfu) {
                    // Enter to DFU OTA mode
                	  gecko_cmd_system_reset(2);
                  } else {
                    // remove connection from active connections
                    removeConnection(evt->data.evt_le_connection_closed.connection);
                    // start scanning again to find new devices
                    BTSTACK_CHECK_RESPONSE(gecko_cmd_le_gap_start_discovery(le_gap_phy_1m, le_gap_discover_generic));
                    connState = scanning;
                  }
                  break;

        // This event is generated when a characteristic value was received e.g. an indication
        case gecko_evt_gatt_characteristic_value_id:

          sprintf(server_a,"%x:%x:%x:%x:%x:%x",ADDRESS.addr[5],ADDRESS.addr[4],ADDRESS.addr[3],ADDRESS.addr[2],ADDRESS.addr[1], ADDRESS.addr[0]);
          LOG_INFO("\n\r%s",server_a);
          displayPrintf(DISPLAY_ROW_BTADDR2,server_a);

          if((evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_button_state))
          {

        	 struct gecko_msg_gatt_read_characteristic_value_rsp_t *button_data;

        	 button_data = gecko_cmd_gatt_read_characteristic_value(evt->data.evt_gatt_server_characteristic_status.connection,gattdb_button_state);
        	 button_data->result;
        	 if(button_data->result == 1)
        	 {
        		 displayPrintf(DISPLAY_ROW_ACTION,"Button Pressed");
        	 }
        	 else
        	 {
        		 displayPrintf(DISPLAY_ROW_ACTION,"Button Released");
        	 }
          }
          else if((evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_temperature_measurement))
          		 {
        	  charValue = &(evt->data.evt_gatt_characteristic_value.value.data[0]);
        	            tableIndex = findIndexByConnectionHandle(evt->data.evt_gatt_characteristic_value.connection);
        	            if (tableIndex != TABLE_INDEX_INVALID) {
        	              connProperties[tableIndex].temperature = (charValue[1] << 0) + (charValue[2] << 8) + (charValue[3] << 16);
        	            }
        	            // Send confirmation for the indication
        	            BTSTACK_CHECK_RESPONSE(gecko_cmd_gatt_send_characteristic_confirmation(evt->data.evt_gatt_characteristic_value.connection));
        	            displayPrintf(DISPLAY_ROW_TEMPVALUE,"%f",(float)connProperties[tableIndex].temperature/1000);
          		 }

          // Trigger RSSI measurement on the connection
          BTSTACK_CHECK_RESPONSE(gecko_cmd_le_connection_get_rssi(evt->data.evt_gatt_characteristic_value.connection));
          break;

        // This event is generated when RSSI value was measured
        case gecko_evt_le_connection_rssi_id:
          tableIndex = findIndexByConnectionHandle(evt->data.evt_le_connection_rssi.connection);
          if (tableIndex != TABLE_INDEX_INVALID) {
            connProperties[tableIndex].rssi = evt->data.evt_le_connection_rssi.rssi;
          }
          // Trigger printing
          gecko_external_signal(EXT_SIGNAL_PRINT_RESULTS);
          break;

        // This event is triggered by an external signal
        case gecko_evt_system_external_signal_id:
        /*if(Gpio_flag == 0 )
         {
      	   //BTSTACK_CHECK_RESPONSE(gecko_cmd_gatt_server_send_characteristic_notification(0xFF, gattdb_button_state, 5, buffer_button));
      	   buffer_button[0]=0;
      	   button_state=0;
      	 }
      	else if(Gpio_flag == 1)
         {
      	   //BTSTACK_CHECK_RESPONSE(gecko_cmd_gatt_server_send_characteristic_notification(0xFF, gattdb_button_state, 5, buffer_button));
      	   buffer_button[0]=1;
      	   button_state=1;
           gecko_cmd_sm_passkey_confirm(evt->data.evt_sm_confirm_passkey.connection,1);
      	 }*/
        break;

        // Check if the user-type OTA Control Characteristic was written.
        // If ota_control was written, boot the device into Device Firmware Upgrade (DFU) mode.
        case gecko_evt_gatt_server_user_write_request_id:
          if (evt->data.evt_gatt_server_user_write_request.characteristic == gattdb_ota_control) {
            // Set flag to enter to OTA mode
            bootToDfu = 1;
            // Send response to Write Request
            BTSTACK_CHECK_RESPONSE(gecko_cmd_gatt_server_send_user_write_response(
              evt->data.evt_gatt_server_user_write_request.connection,
              gattdb_ota_control,
              bg_err_success));
            // Close connection to enter to DFU OTA mode
            BTSTACK_CHECK_RESPONSE(gecko_cmd_le_connection_close(evt->data.evt_gatt_server_user_write_request.connection));
          }
          break;

        default:
          break;
#endif
	}
}

#if DEVICE_IS_BLE_SERVER == 0
void initProperties(void)
{
	  uint8_t i;
	  activeConnectionsNum = 0;

	  for (i = 0; i < MAX_CONNECTIONS; i++) {
	    connProperties[i].connectionHandle = CONNECTION_HANDLE_INVALID;
	    connProperties[i].thermometerServiceHandle = SERVICE_HANDLE_INVALID;
	    connProperties[i].thermometerCharacteristicHandle = CHARACTERISTIC_HANDLE_INVALID;
	    connProperties[i].button= TEMP_INVALID;
	    connProperties[i].buttonServiceHandle= SERVICE_HANDLE_INVALID;
	    connProperties[i].buttonCharacteristicHandle = CHARACTERISTIC_HANDLE_INVALID;
	    connProperties[i].temperature = TEMP_INVALID;
	    connProperties[i].rssi = RSSI_INVALID;
	  }
}

// Parse advertisements looking for advertised Health Thermometer service
uint8_t findServiceInAdvertisement(uint8_t *data, uint8_t len)
{
  uint8_t adFieldLength;
  uint8_t adFieldType;
  uint8_t i = 0;
  // Parse advertisement packet
  while (i < len) {
    adFieldLength = data[i];
    adFieldType = data[i + 1];
    // Partial ($02) or complete ($03) list of 16-bit UUIDs
    if (adFieldType == 0x02 || adFieldType == 0x03) {
      // compare UUID to Health Thermometer service UUID
      if (memcmp(&data[i + 2], thermoService, 2) == 0) {
        return 1;
      }
    }
    // advance to the next AD struct
    i = i + adFieldLength + 1;
  }
  return 0;
}

uint8_t findServiceInButtonAdvertisement(uint8_t *data, uint8_t len)
{
  uint8_t adFieldLength;
  uint8_t adFieldType;
  uint8_t i = 0;
  // Parse advertisement packet
  while (i < len) {
    adFieldLength = data[i];
    adFieldType = data[i + 1];
    // Partial ($02) or complete ($03) list of 16-bit UUIDs
    if (adFieldType == 0x02 || adFieldType == 0x03) {
      // compare UUID to Health Thermometer service UUID
      if (memcmp(&data[i + 2], ButtonService, 16) == 0) {
        return 1;
      }
    }
    // advance to the next AD struct
    i = i + adFieldLength + 1;
  }
  return 0;
}
// Find the index of a given connection in the connection_properties array
uint8_t findIndexByConnectionHandle(uint8_t connection)
{
  for (uint8_t i = 0; i < activeConnectionsNum; i++) {
    if (connProperties[i].connectionHandle == connection) {
      return i;
    }
  }
  return TABLE_INDEX_INVALID;
}

// Add a new connection to the connection_properties array
void addConnection(uint8_t connection, uint16_t address)
{
  connProperties[activeConnectionsNum].connectionHandle = connection;
  connProperties[activeConnectionsNum].serverAddress    = address;
  activeConnectionsNum++;
}

// Remove a connection from the connection_properties array
void removeConnection(uint8_t connection)
{
  uint8_t i;
  uint8_t table_index = findIndexByConnectionHandle(connection);

  if (activeConnectionsNum > 0) {
    activeConnectionsNum--;
  }
  // Shift entries after the removed connection toward 0 index
  for (i = table_index; i < activeConnectionsNum; i++) {
    connProperties[i] = connProperties[i + 1];
  }
  // Clear the slots we've just removed so no junk values appear
  for (i = activeConnectionsNum; i < MAX_CONNECTIONS; i++) {
    connProperties[i].connectionHandle = CONNECTION_HANDLE_INVALID;
    connProperties[i].thermometerServiceHandle = SERVICE_HANDLE_INVALID;
    connProperties[i].thermometerCharacteristicHandle = CHARACTERISTIC_HANDLE_INVALID;
    connProperties[i].temperature = TEMP_INVALID;
    connProperties[i].rssi = RSSI_INVALID;
  }
}
#endif

