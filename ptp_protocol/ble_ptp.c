/*Simple Sync-PTP BLE protocol*/


#include "ptp_ble.h"

#ifndef COPY_VAR
#define COPY_VAR(source,dest) (memcpy((source),(dest),sizeof(dest)))
#endif



/**************************Service Variables*************************/
ptp_uuid_t sync_uuid; /*store the Universally unique identifier*/
ptp_ProfileHandle_t handlers; /*store the service characteristics handler*/
static ptp_ControlTypeDef handler_ptp_app;/*total control of the application handler & app status*/
static uint8_t ptp_role;
static uint8_t max_number_entries;
#if (ROLE== GAP_CENTRAL_ROLE)
ptp_status_table PTPStatus [EXPECTED_NODES];/*form the master */
#elif (ROLE==GAP_PERIPHERAL_ROLE)
ptp_status_table PTPStatus [EXPECTED_CENTRAL_NODES]/*form the slaves*/
#endif
app_service_t bleptp_service;
app_attr_t bleptp_tx_att;
app_attr_t bleptp_rx_att;

/* ptp service*/
static const uint8_t ptp_service_uuid[16] = { 0x66, 0x9a, 0x0c,
		0x20, 0x00, 0x08, 0x96, 0x9e, 0xe2, 0x11, 0x9e, 0xb1, 0xf0, 0xf2, 0x73,
		0xd9 };
static const uint8_t ptp_TXchar_uuid[16] = { 0x66,0x9a,0x0c,
		0x20,0x00,0x08,0x96,0x9e,0xe2,0x11,0x9e,0xb1,0xe1,0xf2,0x73,
		0xd9};
static const uint8_t ptp_RXchar_uuid[16] = { 0x66,
	0x9a,0x0c,0x20,0x00,0x08,0x96,0x9e,0xe2,0x11,0x9e,0xb1,0xe2,0xf2,0x73,
	0xd9};


/******************************Static Func**************************************/
static void init_ptp_profile(app_profile_t * profile);
static void ptp_error_handler(void);
/*******************************************************************************/


/**
  * @brief  This function initialize the ptp_service.
  * @ This function must be called at the begining of the application.
  * @param profile datastructure.
  * @
  */
static void init_ptp_profile(app_profile_t * profile){
/*create the ptp_service */
  COPY_VAR(bleptp_service.ServiceUUID,ptp_service_uuid);
  bleptp_service.service_uuid_type=UUID_TYPE_128;
  bleptp_service.service_type=PRIMARY_SERVICE;
  bleptp_service.max_attr_records=7;
  /*copy the and associate the service to the BLE application profile*/
  ret = APP_add_BLE_Service(profile,&bleptp_service);
  
  if(ret!=APP_SUCCESS) ptp_error_handler();
  /*create the ptp_TX_attribute*/
  COPY_VAR(bleptp_tx_att.CharUUID,ptp_TXchar_uuid);
  bleptp_tx_att.charUuidType = UUID_TYPE_128;
  bleptp_tx_att.charValueLen = 20;
  bleptp_tx_att.charProperties = CHAR_PROP_NOTIFY;
  bleptp_tx_att.secPermissions = ATTR_PERMISSION_NONE;
  bleptp_tx_att.gattEvtMask = GATT_DONT_NOTIFY_EVENTS;
  bleptp_tx_att.encryKeySize=16;
  bleptp_tx_att.isVariable=1;
  /*copy and associate the ptp_TX_attribute to the ptp service*/
   ret= APP_add_BLE_attr(&bleptp_service,&bleptp_tx_att);
    if(ret!=APP_SUCCESS)ptp_error_handler();
  /*create the ptp_RX_attribute*/
   COPY_VAR(bleptp_rx_att.CharUUID,charUuidRX);
  bleptp_rx_att.charUuidType = UUID_TYPE_128;
  bleptp_rx_att.charValueLen = 20;
  bleptp_rx_att.charProperties = CHAR_PROP_NOTIFY;
  bleptp_rx_att.secPermissions = ATTR_PERMISSION_NONE;
  bleptp_rx_att.gattEvtMask = GATT_DONT_NOTIFY_EVENTS;
  bleptp_rx_att.encryKeySize=16;
  bleptp_rx_att.isVariable=1;
  /*copy and associate the RX_attribute to a service*/
  ret= APP_add_BLE_attr(&bleptp_service,&bleptp_rx_att); 
    if(ret!=APP_SUCCESS)ptp_error_handler();
}




void ptp_Dispatch(ptp_fsm * ptp_inst){
	switch(ptp_inst->C_State){
		case INIT:
		/*slave turn on*/
		/*init service, discovery master clock, create a sync, follow req*/

		case UNSYNC:
		/*slave requiere re-sync */
		case SYNC:
		/*slave already sync*/
		case WAIT_RESP:
		/*Slave wait for delay resp*/
		case PENDING_REQ:
		/*Slave have to req delay*/
	}
}

void app_Error_Handler(void){
	
}


/**
  * @brief  This function return the specific ptp status per connection.
  * @param uint16_t connHandler: the connection handler associated to a connection status.
  * @
  */

ptp_state_t * ptp_get_status(uint16_t connHandler){

/*since the number of entries are very small up to 8 
 *it is not necessary to implement a complex hash table to associate
 *the connection handler to the status, however this implementation 
 *must to pay a constant O(n) for search throught the entries. 
 *it is recomended to implement a hash table to reduce this overhead 
 *(in case in which it could be needed)*/
	ptp_state_t * rptp_status = NULL;
uint8_t i;

	for(i=0; i < max_number_entries; i++)
	{
		if(PTPStatus[i].Chandler == connHandler)
		{
			rptp_status = &PTPStatus[i].state;
			break;
		}

	}
return  rptp_status
}

/**
  * @brief  This function initialize the ptp protocol.
  * @param uint8_t ptp_dv_role: define if for this application the device 
  * will be (the master: who has the reference clock) or
  * (the slave: who synchonize its clock to a master reference clock).
  * @param  app_profile_t * profile: profile in where will be associated this application.
  * @
  */
ptp_status_t Init_ptp_application(uint8_t ptp_dv_role, app_profile_t * profile){
	uint8_t i;
	uint8_t expected_nodes;
	/*1. associate the role for this device*/
	ptp_role = ptp_dv_role;
	/*associate this service to the reference profile*/
	init_ptp_profile(profile);
	/*for each spected connection initialize the status connection*/
#if (ROLE == GAP_CENTRAL_ROLE)
	expected_nodes = EXPECTED_NODES;
#elif(ROLE==GAP_PERIPHERAL_ROLE)
	expected_nodes = EXPECTED_CENTRAL_NODES
#endif

	for(i=0; i <expected_nodes;i ++){
		PTPStatus[i].state=UNITIALIZED;
	}

return PTP_SUCESSS;
}


uint8_t ptp_packet_hdr_parse(uint8_t * data,uint8_t data_len, ptp_hdr * hdr){
uint8_t * p;

	if(data_len < 4)
	{
		/*something is wrong return 0*/
		return 0;
	}	

	p= data;

	hdr->ptp_type = p[0]

}

uint8_t create_ptp_packet_hdr(uint16_t chndler,uint8_t type, ptp_hdr * hdr,uint8_t *buff)
{
	uint8_t * p = buff;

	hdr->ptp_type=type;
	hdr->ptp_version = PTP_VERSION;
	hdr->domain_number = 0;
	hdr->control_field = get_control_field(type);
	hdr->sequence_id=get_sequence_id();
	hdr->msg_sync_interval = SYNC_INTERVAL_MS;
	hdr->source_id = chndler;

	p[0] =  ((hdr->ptp_type & 0xF)<< 7) |
			((hdr->ptp_version & 0x1)<<0x4) |
			((hdr->domain_number & 0x1)<<0x3) |
			((hdr->domain_number & 0x3)<<0x2);
	p[1] = hdr->sequence_id;
	
	p[2] = (chndler & 0xFF);

	p[3] = ((chndler >> 0x08)) & 0xFF  

	return 4; 		
}

