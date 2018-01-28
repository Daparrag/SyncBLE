/*sync_control_interface*/
/*
* this interface is used to inform and provide synchronization parameters for BLE applications
* this could take advantage of the ptp_ synchronization mechanism development in a previous relase
*/

/* Operation modes:
*
* a) using the PTP protocol for clock synchronization and timestamp solution
* b) static control mechanism based on the connection interval and
*    connection length configuration.
*
*/


/*Task:
* 1) start the synchronization control process it could evolve  the
	 PTP synchronization app or report to the slaves the static synchronization parameters .

* 2) inform to the applications the status of the synchronization,
	 as well the synchronization parameters used posteriorly to
	 time-stamp the data by the application
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
#include "media_sync_client.h"



static const uint8_t sync_control_service_uuid[16] = { 0x66, 0x9a, 0x0c,
		0x20, 0x00, 0x08, 0x96, 0x9e, 0xe2, 0x11, 0x9f, 0xb1, 0xf0, 0xf2, 0x73,
		0xd9 };


static const uint8_t  sync_control_TXchar_uuid[16] = { 0x66,0x9a,0x0c,
		0x20,0x00,0x08,0x96,0x9e,0xe2,0x11,0x9f,0xb1,0xe1,0xf2,0x73,
		0xd9};

static ctrl_status_table CTRL_SYNC_STR[EXPECTED_NODES];
static ctrl_sync_status  CTRL_status = UNSTARTED;
static app_service_t ctrl_sync_service;
static app_attr_t ctrl_sync_tx_att;
static uint8_t source_id = SOURCE_ID;
static uint8_t total_receivers;
static volatile uint8_t ctrl_peding_packet = 0;
static ctrl_status_table CTRL_GBAL_STR[EXPECTED_NODES];
volatile uint8_t ctrl_sync_id = 0;


/****************STATIC FUNCTIONS****************************/
static void Ctrl_Sync_error_handler(void);

static uint16_t ctrl_get_delay_by_id(uint8_t creceiver_id);

static uint16_t  ctrl_get_max_delay(uint8_t ctrl_table_idx);

static uint8_t ctrl_get_source_id(void);

static uint16_t  ctrl_get_max_delay(uint8_t ctrl_table_idx);

static uint8_t ctrl_get_source_id(void);

static uint8_t ctrl_get_total_receives(void);

static uint8_t create_ctrl_packet_hdr( uint8_t creceiver_id ,
									   uint8_t cpkt_type, 
									   ctrl_sync_hdr * hdr, 
									   uint8_t *buff);

static uint8_t create_ctrl_init_packet( uint8_t creceiver_id, 
										ctrl_init_packet * init_pkt_str, 
										uint8_t *buff );

static ctrl_status_table * get_ctrl_table_by_src_dest_index(uint8_t _idx);

static uint8_t create_ctrl_report_src_packet( uint8_t creceiver_id, 
											  ctrl_report_src_packet * src_report, 
											  uint8_t *buff );

static void process_init_packet(ctrl_init_packet * init_pack);

static void process_report_packet(ctrl_report_src_packet * report_pack);

static void send_ctrl_sync_packet(uint8_t creceiver_id, uint8_t pkt_type);

static uint8_t  parse_ctrl_sync_packet_header(uint8_t * data, 
											  uint8_t data_len, 
											  ctrl_sync_hdr * hdr);

static uint8_t  parse_ctrl_init_packet(uint8_t * data, 
									   uint8_t data_len,
									   ctrl_init_packet * init_pack);

static uint8_t  parse_ctrl_report_src_packet(uint8_t * data, 
											 uint8_t data_len,
											 ctrl_report_src_packet * report_pack);












/**
  * @brief  This function return the tx_delay by the id of the node.
  *	@param uint8_t ctrl_table_idx : id of the source or receiver control structure.
  * @retval : the tx_delay of the receiver.
  */

static uint16_t ctrl_get_delay_by_id(uint8_t creceiver_id)
{
	uint16_t rtx_delay;
#if CTRL_MODE


#else
	rtx_delay = CTRL_GBAL_STR[creceiver_id].tx_delay;
#endif	

	return rtx_delay;

}

static uint8_t ctrl_get_seq_id (uint8_t _idx)
{
  uint8_t seq; 
  
  	if(_idx > EXPECTED_NODES) return 0;
          seq = CTRL_GBAL_STR[_idx].seq_id;
          CTRL_GBAL_STR[_idx].seq_id+=1;
        return seq;
}


/**
  * @brief  This function return the maximum slave delay by the id of the node.
  *	@param uint8_t ctrl_table_idx : id of the source or receiver control structure.
  * @retval : the max_slave_delay of the receiver.
  */
static uint16_t  ctrl_get_max_delay(uint8_t ctrl_table_idx)
{
	uint16_t max_delay;

#if CTRL_MODE


#else
	max_delay = CTRL_GBAL_STR[ctrl_table_idx].slave_max_delay_diff;

#endif	
	return max_delay;

}
static uint8_t ctrl_get_packets_to_tx(uint8_t client_idx)
{
    if(client_idx > EXPECTED_NODES) return 0;
   return  CTRL_GBAL_STR[client_idx].total_packets;
  
}




/**
  * @brief  This function return the source id of the node.
  *	@param none
  * @retval : the control id of the node.
  */

static uint8_t ctrl_get_source_id(void)
{

	return source_id;
}



/**
  * @brief  This function return the number of receivers for a single sync group.
  *	@param none
  * @retval : number of receivers within a synchronization group
  * @note : this version only support one syncgroup.
  */
static uint8_t ctrl_get_total_receives(void)
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
static uint8_t create_ctrl_packet_hdr( uint8_t creceiver_id ,uint8_t cpkt_type, ctrl_sync_hdr * hdr, uint8_t *buff)
{
	uint8_t * p = buff;
	hdr->pkt_type = cpkt_type;
	hdr->source_id = ctrl_get_source_id();
	hdr->receiver_id = creceiver_id;
	hdr->total_receivers = ctrl_get_total_receives();
        hdr->seq_id= ctrl_get_seq_id(creceiver_id);
	*p++=hdr->pkt_type;
	*p++=hdr->source_id;
	*p++=hdr->receiver_id;
	*p++=hdr->total_receivers;

	 return (p - buff);
}



/**
  * @brief  This function creates a control sync init packet.
  * @param uint8_t creceiver_id; 
  *	@param uint8_t cpackets: num_packets to transmit default use 0.
  *	@param ctrl_init_packet * init_pkt_str : control init datastructure
  *	@param uint8_t * buff : packet buffer
  * @retval : payload pointer.
  */
static uint8_t create_ctrl_init_packet( uint8_t creceiver_id,ctrl_init_packet * init_pkt_str, uint8_t *buff )
{
	uint8_t ret;
	uint8_t * p = buff;

	ret = create_ctrl_packet_hdr (creceiver_id,INITIATOR,&(init_pkt_str->header),buff);
	init_pkt_str->total_packets = ctrl_get_packets_to_tx(creceiver_id);
	init_pkt_str->tx_delay = ctrl_get_delay_by_id(creceiver_id);
	init_pkt_str->slave_max_delay = ctrl_get_max_delay(creceiver_id);
	 p += ret;
	*p++ = init_pkt_str->total_packets;
	*p++ = ((init_pkt_str->tx_delay & 0xFF00) >> 8);
	*p++ = (init_pkt_str->tx_delay & 0xFF);
	*p++ = ((init_pkt_str->slave_max_delay & 0xFF00) >> 8);
	*p++ = (init_pkt_str->slave_max_delay & 0xFF);

	return (p - buff); 

}


/**
  * @brief  This function return a ctrl_status_table associate to an source index.
  * @param uint8_t _idx : source or destination index; 
  * @retval ctrl_status_table *:  pointer to the control table index or null i case of error.
  *
  */
static ctrl_status_table * get_ctrl_table_by_src_dest_index(uint8_t _idx){

	if(_idx > EXPECTED_NODES) return NULL;
	return &CTRL_GBAL_STR[_idx];
}





/**
  * @brief  This function creates a control sync report packet.
  * @param uint8_t creceiver_id; 
  *	@param ctrl_report_src_packet * src_report: src report data-structure
  *	@param uint8_t * buff : packet buffer
  * @retval : payload pointer.
  */
static uint8_t create_ctrl_report_src_packet( uint8_t creceiver_id, ctrl_report_src_packet * src_report, uint8_t *buff )
{
	uint8_t ret;
	uint8_t * p = buff;
        
	ret = create_ctrl_packet_hdr (creceiver_id,REPORT_SRC,&(src_report->header),buff);
	src_report->tx_delay = ctrl_get_delay_by_id(creceiver_id);
	src_report->slave_max_delay = ctrl_get_max_delay(creceiver_id);
	p += ret;
	*p++=  ((src_report->tx_delay & 0xFF00) >> 8);
	*p++=  (src_report->tx_delay & 0xFF);
	*p++=	((src_report->slave_max_delay & 0xFF00) >> 8);
	*p++=	(src_report->slave_max_delay & 0xFF);
	return (p - buff); 
}


/**
  * @brief  This function process a control sync init packet.
  * @param ctrl_init_packet * init_pack : input init packet to be processed; 
  * @retval : none.
  */


static void process_init_packet(ctrl_init_packet * init_pack)
{


	CTRL_status = STARTING; /*in theory this has to be assocoate to each source */
	uint8_t source_idx  = init_pack->header.source_id;
	uint8_t node_id     = init_pack->header.receiver_id;
	uint8_t squence_id  = init_pack->header.seq_id;
	uint8_t t_receivers = init_pack->header.total_receivers;
	uint8_t t_packets   = init_pack->total_packets;
	uint16_t tx_delay   = init_pack->tx_delay;
	uint16_t max_delay  = init_pack->slave_max_delay;


	if(source_idx > EXPECTED_NODES)Ctrl_Sync_error_handler(); 

	CTRL_GBAL_STR[source_idx].source_id=source_idx;
	CTRL_GBAL_STR[source_idx].receiver_id=node_id;
	CTRL_GBAL_STR[source_idx].seq_id=squence_id;
	CTRL_GBAL_STR[source_idx].total_receivers=t_receivers;
	CTRL_GBAL_STR[source_idx].total_packets=t_packets;
	CTRL_GBAL_STR[source_idx].tx_delay= tx_delay;
	CTRL_GBAL_STR[source_idx].slave_max_delay_diff=max_delay;

	CTRL_status = IDLE;	

}


/**
  * @brief  This function process a control sync report packet.
  * @param ctrl_report_src_packet * report_pack : input report packet to be processed; 
  * @retval : none.
  */

static void process_report_packet(ctrl_report_src_packet * report_pack)
{
	uint8_t tmp_id;
	ctrl_status_table * temp_table;


	tmp_id = report_pack->header.source_id;
	temp_table = get_ctrl_table_by_src_dest_index(tmp_id);

	if( temp_table->receiver_id == report_pack->header.receiver_id)
	{
		temp_table->tx_delay = report_pack->tx_delay;
		temp_table->slave_max_delay_diff = report_pack->slave_max_delay;
	}

}


/**
  * @brief  This function process the input packets arriving form the BLE interface.
  * @param uint16_t chandler : connection handler associated ;
  *	@param ctrl_report_src_packet * src_report: src report data-structure
  *	@param uint8_t * buff : packet buffer
  * @retval : none.
  */

void ctrl_input_packet_process(uint16_t chandler, 
                            uint16_t attrhandler, 
                            uint8_t data_length, 
                            uint8_t *att_data, 
                            tClockTime arval_time)
{

	uint8_t ret;
	ctrl_sync_hdr ctrl_hdr;
	ctrl_init_packet init_pack;
	ctrl_report_src_packet report_pack;


	if(attrhandler == ctrl_sync_tx_att.Associate_CharHandler+1)
	{

		ret = parse_ctrl_sync_packet_header(att_data,data_length,&ctrl_hdr);
		if(ret==0)Ctrl_Sync_error_handler();
		

		if(ctrl_hdr.pkt_type == INITIATOR)
		{
			init_pack.header = ctrl_hdr;
			ret += parse_ctrl_init_packet(att_data+ret, data_length-ret, &init_pack);
			if(ret==0)Ctrl_Sync_error_handler();
			process_init_packet(&init_pack);

		}else if(ctrl_hdr.pkt_type == REPORT_SRC)
		{
			report_pack.header = ctrl_hdr;
			ret += parse_ctrl_report_src_packet(att_data+ret,data_length-ret,&report_pack);
			if(ret==0)Ctrl_Sync_error_handler();
			process_report_packet(&report_pack);
		}




	}

}


/**
  * @brief  This function send a control sync init or report packet.
  * @param uint8_t creceiver_id: destination ID; 
  *	@param  uint8_t pkt_type : packet type
  * @retval : void.
  */

static void send_ctrl_sync_packet(uint8_t creceiver_id, uint8_t pkt_type)
{
	tBleStatus res_ble;
	uint8_t ret;
	uint8_t tx_buffer[10];
	ctrl_init_packet init_pck;
	ctrl_report_src_packet src_report_pck;

	if(pkt_type == INITIATOR)
	{

		ret = create_ctrl_init_packet(creceiver_id,&init_pck,tx_buffer);

	}else if (pkt_type == REPORT_SRC){

		ret = create_ctrl_report_src_packet(creceiver_id,&src_report_pck,tx_buffer);

	}

	if(ret==0)Ctrl_Sync_error_handler();
	res_ble = aci_gatt_update_char_value(ctrl_sync_service.ServiceHandle,ctrl_sync_tx_att.CharHandle,0,ret,tx_buffer);
	 if(res_ble!= BLE_STATUS_SUCCESS)Ctrl_Sync_error_handler(); 

}



/**
  * @brief  This function parse and input control-sync init or report packet header.
  * @param  uint8_t * data : buffer input data.
  *	@param  uint8_t data_len : total length of the input data.
  *	@param  ctrl_sync_hdr * hdr : header data_structure. 
  * @retval : payload pointer offset.
  */
static uint8_t  parse_ctrl_sync_packet_header(uint8_t * data, uint8_t data_len, ctrl_sync_hdr * hdr)
{
	uint8_t * p;

	if(data_len < 4)
	{
		/*something is wrong return 0*/
		return 0;
	}

	p=data;

	hdr->pkt_type = 		p[0];
	hdr->source_id =		p[1];
	hdr->receiver_id =		p[2];
	hdr->total_receivers =  p[3];

	return 4; 

}


/**
  * @brief  This function parse and input control-sync init packet header.
  * @param  uint8_t * data : buffer input data.
  *	@param  uint8_t data_len : total length of the input data.
  *	@param  ctrl_init_packet * ctrl_pack : init_data_structure. 
  * @retval : payload pointer offset.
  */
static uint8_t  parse_ctrl_init_packet(uint8_t * data, uint8_t data_len,ctrl_init_packet * init_pack)
{
	uint8_t ret=0;
	uint8_t * p = data;
	
	init_pack->total_packets = p[0];

	init_pack->tx_delay = 	(p[1] & 0x00FF << 8) |
						  	(p[2] & 0xFF);

	init_pack->slave_max_delay = (p[3] & 0x00FF << 8) |
								 (p[4] & 0xFF);

	ret+=5;
        return ret;
}


/**
  * @brief  This function parse and input control-sync source report packet header.
  * @param  uint8_t * data : buffer input data.
  *	@param  uint8_t data_len : total length of the input data.
  *	@param  ctrl_report_src_packet * ctrl_pack : source_report_data_structure. 
  * @retval : payload pointer offset.
  */
static uint8_t  parse_ctrl_report_src_packet(uint8_t * data, uint8_t data_len,ctrl_report_src_packet * report_pack)
{
	uint8_t ret=0;
	uint8_t * p = data;
	

	report_pack->tx_delay = 	(p[0] & 0x00FF << 8) |
						  		(p[1] & 0xFF);

	report_pack->slave_max_delay = (p[2] & 0x00FF << 8) |
								   (p[3] & 0xFF);

	ret+=4;	
        
        return ret;
}



/**
  * @brief  This function initializes the contol synchonization protocol.
  * @param  app_profile_t * profile : profile to associate the service. 
  * @retval : none.
  */

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
/*INITIALIZE THE CONNECTION INTERVAL INTERRUPTION USED TO SEND DATA SYNCHRONOUSLY*/
	 BlueNRG_ConnInterval_Init(10);
#endif	

}

/**
  * @brief  This function return the status of the synchronization protocol.
  * @param  none
  * @retval : control synchronization status.
  */

ctrl_sync_status Get_Ctrl_Sync_status(void)
{
	return CTRL_status;
}

/**
  * @brief  This function force a specific status of the synchonization protocol.
  * @param  ctrl_sync_status status: control synchonization status
  * @retval : none
  */
void Ctrl_Sync_set_status(ctrl_sync_status status)
{
	CTRL_status = status;
}  




/**
  * @brief  This function is used at the beginning of the media transmission
  			to sent the initialization parameters to all the synchronization-media-clients.
  * @param  uint8_t no_receivers : indicates the number of receivers.
  * @param  uint8_t no_packets : optional number of packets to transmit.
  * @retval : none
  */
void Ctrl_Sync_start(uint8_t no_receivers, uint8_t no_packets)
{
	uint8_t i;		
	if(CTRL_status != UNSTARTED) return;
	
#if CTRL_MODE
/*using ptp*/

#else
/*not_ptp then uses the static config*/	
total_receivers = no_receivers;
	for(i=0; i < no_receivers; i++)
	{	
		CTRL_GBAL_STR[i].source_id = ctrl_get_source_id();
		CTRL_GBAL_STR[i].receiver_id = i+1;
		CTRL_GBAL_STR[i].seq_id = 0;
		CTRL_GBAL_STR[i].total_receivers=no_receivers;
		CTRL_GBAL_STR[i].total_packets = no_packets;
		CTRL_GBAL_STR[i].tx_delay = TX_INTERVAL;
		CTRL_GBAL_STR[i].slave_max_delay_diff = EXPECTED_DELAY * (1-i);
	}

		ctrl_peding_packet = 1;
		CTRL_status = STARTING;


#endif

	
}


void Init_Ctrl_Sync_application(app_profile_t * profile)
{
	/*1. associate the service to the reference profile*/
	/*1.1. init the connection interval interruption*/

	Ctrl_Sync_init(profile);
}





void Ctrl_Sync_send_pending_packets(void)
{

uint8_t i;

	if(ctrl_peding_packet)
	{
		switch (CTRL_status)
		{
			case STARTING:
			{
				CTRL_status = SYNCRONIZING;
				for(i=0; i < total_receivers; i++)
					send_ctrl_sync_packet(i,INITIATOR);
				ctrl_peding_packet=0;			
				CTRL_status = IDLE;
			}
			break;
			case IDLE:
			{
				CTRL_status = SYNCRONIZING;
				for(i=0; i < total_receivers; i++)
					send_ctrl_sync_packet(i,REPORT_SRC);
				ctrl_peding_packet=0;
				CTRL_status = IDLE;

			}

		}
	}
			
}





void Ctrl_Sync_dynamic_sync(void)
{
	/*used to support the PTP protocol*/
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
	tx_buff[6]=((CTRL_SYNC_STR[id_receiver].slave_max_delay_diff & 0xFF00) >> 8);
	tx_buff[7]=((CTRL_SYNC_STR[id_receiver].slave_max_delay_diff) & 0xFF);
        CTRL_SYNC_STR[id_receiver].seq_id +=1;
        res_ble = aci_gatt_update_char_value(ctrl_sync_service.ServiceHandle,ctrl_sync_tx_att.CharHandle,0,8,tx_buff);
         if(res_ble!= BLE_STATUS_SUCCESS)Ctrl_Sync_error_handler(); 
	// res_ble = aci_gatt_write_without_response(chandler,ctrl_sync_tx_att.Associate_CharHandler + 1,8,tx_buffer);
	
}




void Ctrl_Sync_client_process(){

event_t * event;
event = (event_t *)HCI_Get_Event_CB();

if(event!=NULL && event->event_type == EVT_BLUE_GATT_ATTRIBUTE_MODIFIED){

		uint8_t hw_version = get_harware_version();
        if(hw_version==IDB05A1){
	         
	          evt_gatt_attr_modified_IDB05A1 *evt = (evt_gatt_attr_modified_IDB05A1*)event->evt_data;
	          ctrl_input_packet_process(
	          	evt->conn_handle,
                evt->attr_handle,
                evt->data_length,
                evt->att_data,
                event->ISR_timestamp
	          	);
	      }

	}



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






















































































































