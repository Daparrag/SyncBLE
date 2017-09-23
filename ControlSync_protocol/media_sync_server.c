/*sync_control_interface*/
/*
* this interface is used to inform and provide synhonization parameters for BLE applications
* this could take advantage of the ptp_ synchonization mechanims development in a previous relase
*/

/* Operation modes:
*
* a) using the ptp protocol for clock synchonization and timestamp solution
* b) static control mechanims based on the connection interval and 
*    connection length configuration.
*
*/


/*Task:
* 1) start the synchonization control process it could envolve  the 
	 PTP synchonization app or report to the slaves the static synchonization paramenters .

* 2) inform to the appliations the status of the synchonization, 
	 as well the synchonization parameters used posteriously to 
	 timestamp the data by the application
*/



#ifdef BLE_APP_CONFIG
#include  <blefw_conf.h>
#endif


#include "ptp_core.h"
#include "app_ble.h"
#include "ble_firmware.h"
#include "common_tables.h"
#include "clock.h"
#include "ptp_interrupt.h"
#include "stm32f4xx_nucleo_add_led.h"
#include "media_sync_server.h"



static const uint8_t sync_control_service_uuid[16] = { 0x66, 0x9a, 0x0c,
		0x20, 0x00, 0x08, 0x96, 0x9e, 0xe2, 0x11, 0x9f, 0xb1, 0xf0, 0xf2, 0x73,
		0xd9 };


static const uint8_t  sync_control_TXchar_uuid[16] = { 0x66,0x9a,0x0c,
		0x20,0x00,0x08,0x96,0x9e,0xe2,0x11,0x9f,0xb1,0xe1,0xf2,0x73,
		0xd9};

static cstr_ctrl_sync CTRL_SYNC_STR[EXPECTED_NODES];
static ctrl_sync_status  CTRL_status = UNSTARTED;
static app_service_t ctrl_sync_service;
static app_attr_t ctrl_sync_tx_att;
static uint8_t source_id = SOURCE_ID;
static uint8_t total_receivers;
static volatile uint8_t ctrl_peding_packet = 0;




void Ctrl_Sync_error_handler(void);

static uint8_t ctrl_get_source_id()
{

	return source_id;
}

static uint8_t ctrl_get_total_receives()
{

	return total_receivers;
}

/**
  * @brief  This function creates a control sync header.
  * @param uint8_t creceiver_id; 
  *	@param uint8_t cpkt_type: packet type.
  *	@param ctrl_sync_hdr * hdr : ptp_header data-structure
  *	@param uint8_t * buff : packet buffer
  * @retval : payload pointer.
  */
uint8_t create_ctrl_packet_hdr( uint8_t creceiver_id ,uint8_t cpkt_type, ctrl_sync_hdr * hdr, uint8_t *buff)
{
	uint8_t * p = buff;
	hdr->pkt_type = cpkt_type;
	hdr->source_id = ctrl_get_source_id();
	hdr->receiver_id = creceiver_id;
	hdr->total_receivers = ctrl_get_total_receives();
	p[0]=hdr->pkt_type;
	p[1]=hdr->source_id;
	p[2]=hdr->receiver_id;
	p[3]=hdr->total_receivers;

	 return 4;
}

uint8_t create_ctrl_init_packet()
{
	
}




void Ctrl_Sync_init(app_profile_t * profile)
{
  APP_Status ret; 
#if CTRL_MODE


#else
	/*1. configure the Ctrl_Sync service and associate it to the app_profile*/
	COPY_VAR(ctrl_sync_service.ServiceUUID,sync_control_service_uuid);
	ctrl_sync_service.service_uuid_type=UUID_TYPE_128;
  	ctrl_sync_service.service_type=PRIMARY_SERVICE;
  	ctrl_sync_service.max_attr_records=7;
  	/*copy the and associate the service to the BLE application profile*/
  	ret = APP_add_BLE_Service(profile,&ctrl_sync_service);
  	if(ret!=APP_SUCCESS) Ctrl_Sync_error_handler();
  	/*1. configure the  Ctrl_Sync attribute and associate it to the Ctrl_Sync service*/
  	COPY_VAR(ctrl_sync_tx_att.CharUUID,sync_control_TXchar_uuid);
  	ctrl_sync_tx_att.charUuidType = UUID_TYPE_128;
  	ctrl_sync_tx_att.charValueLen = 20;
  	ctrl_sync_tx_att.charProperties = CHAR_PROP_NOTIFY;
  	ctrl_sync_tx_att.secPermissions = ATTR_PERMISSION_NONE;
  	ctrl_sync_tx_att.gattEvtMask = GATT_DONT_NOTIFY_EVENTS;
  	ctrl_sync_tx_att.encryKeySize=16;
  	ctrl_sync_tx_att.isVariable=1;
  	ret= APP_add_BLE_attr(&ctrl_sync_service,&ctrl_sync_tx_att);
    if(ret!=APP_SUCCESS)Ctrl_Sync_error_handler();

#endif	

}



//static void set_ctrl_parameters(ptp_sync_times * ptp_times)
//{
//
//}


//static void Ctrl_Sync_create_packet(uint8_t * buffer,str_ctrl_frame * control_frame )
//{
//
//}


//void Ctrl_Sync_deinit(app_profile_t * profile, Ctrl_config * ptp_config)
//{
//#if CTRL_MODE
//
//#else
//
//#endif		
//}
//
//void Ctrl_Sync_start_session(uint8_t no_receivers, uint8_t no_packets){
//	session_conf.session_id += 1;
//	session_conf.total_receivers = no_receivers;
//	session_conf.total_packets = no_packets;
//
//}

ctrl_sync_status Ctrl_Sync_status(void)
{
	return CTRL_status;
}

void Ctrl_Sync_set_status(ctrl_sync_status status)
{
	CTRL_status = status;
}  


void Ctrl_Sync_start(uint8_t no_receivers, uint8_t no_packets)
{
uint8_t i;
total_receivers = no_receivers;
#if CTRL_MODE

#else
CTRL_status = STARTING;
 
	/*assumming only two slaves*/
for(i=0; i < no_receivers; i++)
{
CTRL_SYNC_STR[i].receiver_id= i;
CTRL_SYNC_STR[i].total_receivers = no_receivers;
CTRL_SYNC_STR[i].total_packets = no_packets;
CTRL_SYNC_STR[i].tx_delay = TX_INTERVAL;
CTRL_SYNC_STR[i].inter_slave_delay =  EXPECTED_DELAY * (1-i);
}
ctrl_peding_packet = 1;
#endif

}


void Ctrl_Sync_parameters(void)
{

}

void Ctrl_Sync_process(void)
{

}




void Ctrl_Sync_send_packet(uint8_t id_receiver)
{
	tBleStatus res_ble;
        uint8_t tx_buff[8];
        
	tx_buff[0]=CTRL_SYNC_STR[id_receiver].receiver_id;
        tx_buff[1]=CTRL_SYNC_STR[id_receiver].seq_id;
	tx_buff[2]=CTRL_SYNC_STR[id_receiver].total_receivers;
	tx_buff[3]=CTRL_SYNC_STR[id_receiver].total_packets;
	tx_buff[4]=((CTRL_SYNC_STR[id_receiver].tx_delay & 0xFF00) >> 8);
	tx_buff[5]=((CTRL_SYNC_STR[id_receiver].tx_delay) & 0xFF);
	tx_buff[6]=((CTRL_SYNC_STR[id_receiver].inter_slave_delay & 0xFF00) >> 8);
	tx_buff[7]=((CTRL_SYNC_STR[id_receiver].inter_slave_delay) & 0xFF);
        CTRL_SYNC_STR[id_receiver].seq_id +=1;
        res_ble = aci_gatt_update_char_value(ctrl_sync_service.ServiceHandle,ctrl_sync_tx_att.CharHandle,0,8,tx_buff);
         if(res_ble!= BLE_STATUS_SUCCESS)Ctrl_Sync_error_handler(); 
	// res_ble = aci_gatt_write_without_response(chandler,ctrl_sync_tx_att.Associate_CharHandler + 1,8,tx_buffer);
	
}

void Ctrl_Sync_connection_interval_handler(){
  uint8_t i;
 if(ctrl_peding_packet){
 	CTRL_status = SYNCRONIZING;
 	for(i=0; i < total_receivers; i++)
 	{
 		Ctrl_Sync_send_packet(i);
 	}
 	ctrl_peding_packet=0;
 	CTRL_status = IDLE;
 }

}




void Ctrl_Sync_error_handler(void)
{

	while(1)
	{

	}
}






















































































































