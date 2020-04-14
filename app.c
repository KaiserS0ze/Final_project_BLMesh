/***************************************************************************//**
 * @file
 * @brief Application code
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

/* C Standard Library headers */
#include <stdio.h>

/* Bluetooth stack headers */
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"
#include "mesh_generic_model_capi_types.h"
#include "mesh_lib.h"

/* GPIO peripheral library */
#include <em_gpio.h>
/* Own header */
#include "app.h"
#include "src/display.h"
#include "src/gpio.h"
#include "src/ble_mesh_device_type.h"
#include "src/log.h"
/// Flag for indicating DFU Reset must be performed

#define TIMER_ID_RESTART          78
//handlke for factory reset
#define TIMER_ID_FACTORY_RESET    77
//handle for provisioning case
#define TIMER_ID_PROVISIONING     66

static uint8_t boot_to_dfu = 0;
//extern uint8_t flag;
int conn_handle;
uint8_t button_state=0x00;
volatile uint8_t buffer_button[1]={0};
volatile uint8_t Button_event=0;
extern volatile uint8_t Gpio_flag;
static uint16_t _elem_index=0xffff;
/***********************************************************************************************//**
 * @addtogroup Application
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup app
 * @{
 **************************************************************************************************/

/*******************************************************************************
 * Initialize used bgapi classes for server.
 ******************************************************************************/

static void client_server_request(uint16_t model_id,
                            uint16_t element_index,
                            uint16_t client_addr,
                            uint16_t server_addr,
                            uint16_t appkey_index,
                            const struct mesh_generic_request *req,
                            uint32_t transition_ms,
                            uint16_t delay_ms,
                            uint8_t request_flags)
{
	//if the value by the switch on publisher pressed gives 0
	if(!(req->on_off))
	{
		  displayPrintf(DISPLAY_ROW_TEMPVALUE,"%s","BUTTON PRESSED");
	}
	//if the value by the switch on publisher not pressed gives 1
	else
	{
		  displayPrintf(DISPLAY_ROW_TEMPVALUE,"%s","BUTTON RELEASED");
	}
}
static void onoff_change(uint16_t model_id,
                         uint16_t element_index,
                         const struct mesh_generic_state *current,
                         const struct mesh_generic_state *target,
                         uint32_t remaining_ms)
{
LOG_WARN("  Inside onoff function  ");
}
static void onoff_recall(uint16_t model_id,
                         uint16_t element_index,
                         const struct mesh_generic_state *current,
                         const struct mesh_generic_state *target,
                         uint32_t transition_ms)
{
	LOG_WARN("  Inside onoff recall  ");
}
void gecko_bgapi_classes_init_server_friend(void)
{
  gecko_bgapi_class_dfu_init();
  gecko_bgapi_class_system_init();
  gecko_bgapi_class_le_gap_init();
  gecko_bgapi_class_le_connection_init();
  //gecko_bgapi_class_gatt_init();
  gecko_bgapi_class_gatt_server_init();
  gecko_bgapi_class_hardware_init();
  gecko_bgapi_class_flash_init();
  gecko_bgapi_class_test_init();
  //gecko_bgapi_class_sm_init();
  gecko_bgapi_class_mesh_node_init();
  //gecko_bgapi_class_mesh_prov_init();
  gecko_bgapi_class_mesh_proxy_init();
  gecko_bgapi_class_mesh_proxy_server_init();
  //gecko_bgapi_class_mesh_proxy_client_init();
  //gecko_bgapi_class_mesh_generic_client_init();
  gecko_bgapi_class_mesh_generic_server_init();
  //gecko_bgapi_class_mesh_vendor_model_init();
  //gecko_bgapi_class_mesh_health_client_init();
  //gecko_bgapi_class_mesh_health_server_init();
  //gecko_bgapi_class_mesh_test_init();
  //gecko_bgapi_class_mesh_lpn_init();
  gecko_bgapi_class_mesh_friend_init();
  gecko_bgapi_class_mesh_lc_server_init();
  gecko_bgapi_class_mesh_lc_setup_server_init();
  gecko_bgapi_class_mesh_scene_server_init();
  gecko_bgapi_class_mesh_scene_setup_server_init();
}

//setting the device name and address for the publisher
void set_device_name_Publisher(bd_addr *pAddr)
{
  char name[9];
  uint16 res;
  // create unique device name using the last two bytes of the Bluetooth address
  sprintf(name, "5823PUB %02x:%02x", pAddr->addr[1], pAddr->addr[0]);
  LOG_DEBUG("Device name: '%s'\r\n", name);
  displayPrintf(DISPLAY_ROW_NAME,"%s",name);

  // write device name to the GATT database
  res = gecko_cmd_gatt_server_write_attribute_value(gattdb_device_name, 0, strlen(name), (uint8 *)name)->result;
  if (res)
  {
    LOG_DEBUG("gecko_cmd_gatt_server_write_attribute_value() failed, code %x\r\n", res);
  }
 }
//setting the device name and address for the subscriber
void set_device_name_Subscriber(bd_addr *pAddr)
{
  char name[9];
  uint16 res;
  sprintf(name, "5823SUB %02x:%02x", pAddr->addr[1], pAddr->addr[0]);
  LOG_DEBUG("Device name: '%s'\r\n", name);
  displayPrintf(DISPLAY_ROW_NAME,"%s",name);

  res = gecko_cmd_gatt_server_write_attribute_value(gattdb_device_name, 0, strlen(name), (uint8 *)name)->result;
  if (res)
  {
    LOG_DEBUG("gecko_cmd_gatt_server_write_attribute_value() failed, code %x\r\n", res);
  }
 }

/*******************************************************************************
 * Initialize used bgapi classes for server.
 ******************************************************************************/
void gecko_bgapi_classes_init_client_lpn(void)
{
	gecko_bgapi_class_dfu_init();
	gecko_bgapi_class_system_init();
	gecko_bgapi_class_le_gap_init();
	gecko_bgapi_class_le_connection_init();
	//gecko_bgapi_class_gatt_init();
	gecko_bgapi_class_gatt_server_init();
	gecko_bgapi_class_hardware_init();
	gecko_bgapi_class_flash_init();
	gecko_bgapi_class_test_init();
	//gecko_bgapi_class_sm_init();
	gecko_bgapi_class_mesh_node_init();
	//gecko_bgapi_class_mesh_prov_init();
	gecko_bgapi_class_mesh_proxy_init();
	gecko_bgapi_class_mesh_proxy_server_init();
	//gecko_bgapi_class_mesh_proxy_client_init();
	gecko_bgapi_class_mesh_generic_client_init();
	//gecko_bgapi_class_mesh_generic_server_init();
	//gecko_bgapi_class_mesh_vendor_model_init();
	//gecko_bgapi_class_mesh_health_client_init();
	//gecko_bgapi_class_mesh_health_server_init();
	//gecko_bgapi_class_mesh_test_init();
	gecko_bgapi_class_mesh_lpn_init();
	//gecko_bgapi_class_mesh_friend_init();
	gecko_bgapi_class_mesh_scene_client_init();
}

/*******************************************************************************
 * Handling of stack events. Both Bluetooth LE and Bluetooth mesh events
 * are handled here.
 * @param[in] evt_id  Incoming event ID.
 * @param[in] evt     Pointer to incoming event.
 ******************************************************************************/
/**************************** INSTRUCTIONS ************************************
 * 1. Before proceeding with assignment profile the project with attached blue
 * gecko and verify if it is being scanned by mobile mesh App.
 * 2. Use Bluetooth Mesh app from Silicon labs for the same and if you are not
 * able to get the app working checkout nRF Mesh App on play store.
 * 3. Add the necessary events for the mesh in switch (evt_id) similar to the
 * BLE assignments.
 * 4. Use the following pdf for reference
 * https://www.silabs.com/documents/public/reference-manuals/bluetooth-le-and-mesh-software-api-reference-manual.pdf
 * 5. Remember to check and log the return status for every Mesh API used.
 * 6. You can take the hints from light and switch example for mesh to know which
 * commands and events are needed and to understand the flow.
 ******************************************************************************/
void handle_ecen5823_gecko_event(uint32_t evt_id, struct gecko_cmd_packet *evt)
{

	uint16_t result;
	static uint8_t trid=0;

	char buf[30];
  if (NULL == evt) {
    return;
  }

  switch (evt_id) {
    case gecko_evt_system_boot_id:
    if (GPIO_PinInGet(button_port,button_pin) == 0 || GPIO_PinInGet(button_one_port,button_one_pin) == 0)
    {

	      LOG_DEBUG("factory reset push button\r\n");

  	  gecko_cmd_flash_ps_erase_all();
  	  gecko_cmd_hardware_set_soft_timer(2 * 32768, TIMER_ID_FACTORY_RESET, 1);
  	  displayPrintf(DISPLAY_ROW_ACTION,"%s","FACTORY RESET");

    }
    	else
    	{
    		LOG_DEBUG("getting device name\r\n");

    			    	  struct gecko_msg_system_get_bt_address_rsp_t *pAddr = gecko_cmd_system_get_bt_address();
    			    	  if(evt->data.evt_mesh_node_initialized.provisioned && DeviceIsOnOffPublisher())
    			    	  {
    			    		  set_device_name_Publisher(&pAddr->address);
    			    	  }
    			    	  if(evt->data.evt_mesh_node_initialized.provisioned && DeviceIsOnOffSubscriber())
    			    	  {
    			    		  set_device_name_Subscriber(&pAddr->address);

    			    	  }
    			          result = gecko_cmd_mesh_node_init()->result;
    			      	  if(result)
    			      	  {
    			      		  sprintf(buf, "init failed (0x%x)", result);
    			      	  }
    	}

#if DEVICE_IS_ONOFF_PUBLISHER
    	displayPrintf(DISPLAY_ROW_NAME,"Publisher");
#else
    	displayPrintf(DISPLAY_ROW_NAME,"Subscriber");
#endif
      bd_addr adr=gecko_cmd_system_get_bt_address()->address;
	  char tem[18];
	  sprintf(tem,"%x:%x:%x:%x:%x:%x",adr.addr[5],adr.addr[4],adr.addr[3],adr.addr[2],adr.addr[1], adr.addr[0]);
	  displayPrintf(DISPLAY_ROW_BTADDR,tem);
      break;

    case gecko_evt_mesh_node_initialized_id:

    	if(evt->data.evt_mesh_node_initialized.provisioned && DeviceUsesClientModel())
    	{
    		LOG_INFO("\n\r Client Init");
    		gecko_cmd_mesh_generic_client_init();
    	}
    	if(evt->data.evt_mesh_node_initialized.provisioned && DeviceUsesServerModel())
    	{
    		LOG_INFO("\n\r Server Init");
    		gecko_cmd_mesh_generic_server_init();
    	}
    	if(evt->data.evt_mesh_node_initialized.provisioned && DeviceIsOnOffPublisher())
    	{
    		mesh_lib_init(malloc,free,8);
    	}
    	if(evt->data.evt_mesh_node_initialized.provisioned && DeviceIsOnOffSubscriber())
    	{
    		mesh_lib_init(malloc,free,9);
    		mesh_lib_generic_server_register_handler(MESH_GENERIC_ON_OFF_SERVER_MODEL_ID,0,client_server_request,0,0);
    		mesh_lib_generic_server_publish(MESH_GENERIC_ON_OFF_SERVER_MODEL_ID,0,mesh_generic_state_on_off);

    	}
    	struct gecko_msg_mesh_node_initialized_evt_t *pData = (struct gecko_msg_mesh_node_initialized_evt_t *)&(evt->data);
    	if (pData->provisioned)
    	{
    		    	LOG_INFO("node is provisioned. address:%x, ivi:%ld\r\n", pData->address, pData->ivi);
    		        _elem_index = 0;
    	}
    	else
    	{
    		    	LOG_INFO("node is unprovisioned\r\n");
    		    	LOG_INFO("starting unprovisioned beaconing...\r\n");
    		    	displayPrintf(DISPLAY_ROW_ACTION,"Unprovisioned");
    		        gecko_cmd_mesh_node_start_unprov_beaconing(0x3);   // enable ADV and GATT provisioning bearer
    	}
    	  break;

    case gecko_evt_hardware_soft_timer_id:

		  switch (evt->data.evt_hardware_soft_timer.handle)
	      {
	        case TIMER_ID_FACTORY_RESET:
	          // reset the device to finish factory reset
	          gecko_cmd_system_reset(0);
	          break;
	        default:
	        	break;
	      }
    	break;

    case gecko_evt_mesh_node_provisioned_id:
    	displayPrintf(DISPLAY_ROW_ACTION,"Provisioned");
    	break;

    case gecko_evt_mesh_node_provisioning_failed_id:
    	displayPrintf(DISPLAY_ROW_ACTION,"Provision Fail");
    	break;

    case gecko_evt_mesh_node_provisioning_started_id:
    	displayPrintf(DISPLAY_ROW_ACTION,"Provisioning");
    	gecko_cmd_hardware_set_soft_timer(32768 / 4, TIMER_ID_PROVISIONING, 0);
        break;

    case gecko_evt_mesh_generic_server_client_request_id:
    	if(DeviceUsesServerModel())
    	{
    		mesh_lib_generic_server_event_handler(evt);
    	}
		break;

    case gecko_evt_mesh_generic_server_state_changed_id:
    	if(DeviceUsesServerModel())
    	{
    		mesh_lib_generic_server_event_handler(evt);
    	}
    	break;

    case gecko_evt_le_connection_opened_id:
    	conn_handle = evt->data.evt_le_connection_opened.connection;
    	displayPrintf(DISPLAY_ROW_CONNECTION,"Connected");
    	break;

    case gecko_evt_mesh_node_reset_id:
    	 if (conn_handle != 0xFF)
    	 {
    	    gecko_cmd_le_connection_close(conn_handle);
    	 }
    	    gecko_cmd_flash_ps_erase_all();
    	break;

    case gecko_evt_le_connection_closed_id:
      /* Check if need to boot to dfu mode */
    	displayPrintf(DISPLAY_ROW_CONNECTION,"");
      if (boot_to_dfu) {
        /* Enter to DFU OTA mode */
        gecko_cmd_system_reset(2);
      }
      break;

    case gecko_evt_gatt_server_user_write_request_id:
      if (evt->data.evt_gatt_server_user_write_request.characteristic == gattdb_ota_control) {
        /* Set flag to enter to OTA mode */
        boot_to_dfu = 1;
        /* Send response to Write Request */
        gecko_cmd_gatt_server_send_user_write_response(
          evt->data.evt_gatt_server_user_write_request.connection,
          gattdb_ota_control,
          bg_err_success);

        /* Close connection to enter to DFU OTA mode */
        gecko_cmd_le_connection_close(evt->data.evt_gatt_server_user_write_request.connection);
      }
      break;

    case gecko_evt_system_external_signal_id:
    	if(Gpio_flag == 1)
    	{
    	Gpio_flag =0;
		LOG_DEBUG("SEND ON OFF REQUEST");
		uint16 resp;
		uint16 delay=0;
		struct mesh_generic_request req;
		const uint32 transtime = 0;
		req.kind = mesh_generic_request_on_off;
		req.on_off = GPIO_PinInGet(button_port, button_pin) ? MESH_GENERIC_ON_OFF_STATE_ON : MESH_GENERIC_ON_OFF_STATE_OFF;
		resp = mesh_lib_generic_client_publish(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,_elem_index,
		     trid,
		     &req,
		     transtime,
		     delay,
		     0
		     );
		     trid++;
		     if (resp)
		     {
		    	 LOG_INFO("gecko_cmd_mesh_generic_client_publish failed,code %x\r\n", resp);
		     }
		     else
		     {
		    	 LOG_INFO("request sent, trid = %u, delay = %d\r\n", trid, delay);
		     }
    	}
		break;
  }
}

