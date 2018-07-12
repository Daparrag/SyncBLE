/*Simple Sync-PTP BLE protocol*/
/*

.%%%%%...%%%%%%..%%%%%...........%%%%%...%%......%%%%%%.
.%%..%%....%%....%%..%%..........%%..%%..%%......%%.....
.%%%%%.....%%....%%%%%...........%%%%%...%%......%%%%...
.%%........%%....%%..............%%..%%..%%......%%.....
.%%........%%....%%......%%%%%%..%%%%%...%%%%%%..%%%%%%.
........................................................

 +----------------------+             +--------------------------+
 |    PTP_SERVER        |             |   PTP_CLIENT             |
 |                      |             |                          |
 |                      |             |     +---------------+    |
 |                      |    Notify   |     |TXchar: DELAY  |    |
 |                     <--------------------+        REQUEST|    |
 |                      |             |     +---------------+    |
 |                      |             |                          |
 |                      |             |     +---------------+    |
 |                      |  Write_req  |     |RXchar:        |    |
 |                      +-----------------> |SYNC           |    |
 |                      |             |     |FOLLOW         |    |
 |                      |             |     |DELAY_RESP     |    |
 |                      |             |     |Notify nable   |    |
 |                      |             |     +---------------+    |
 |                      |             |                          |
 +----------------------+             +--------------------------+

  PTP_SERVER             t0                         t3
  +-------------------------------------------------^---------------+
                          \     \                 /        \
                           \     \             DELAY_REQ    \
                            SYNC  Follow(t0)    /           DELAY_RESP(t3)
  PTP_CLIENT                 \     \           /              \
  +---------------------------v-----v--------------------------v----+
                                    t1        t2
  Delay: (t1-t0)+(t3-t2)/2
  Offset: (t1-t0)-Delay

*/





#ifdef BLE_APP_CONFIG
#include  <blefw_conf.h>
#endif


#include "ptp_core.h"
#include "app_ble.h"
#include "ble_firmware.h"
#include "common_tables.h"
#include "clock_interface.h"
#include "ptp_interrupt.h"
#include "ptp_interrupt.h"

#ifdef TEST_PTP
#include "stm32f4xx_nucleo_add_led.h"
#endif

#define PRINT_PTP 0
#define Cinterval 0


#ifndef COPY_VAR
#define COPY_VAR(source,dest) (memcpy((source),(dest),sizeof(dest)))
#endif


/**************************Service Variables*************************/
uint8_t ptp_role;
static uint8_t max_number_entries;
volatile uint8_t sync_success = FALSE;
static uint8_t sync_interval;
volatile uint8_t Cinterval_started = 0;
static ptp_op_mode ptp_mode = PTP_STATIC; 



#if (ROLE==GAP_CENTRAL_ROLE)
static ptp_status_table PTPStatus [EXPECTED_NODES];/*form the master */
#elif (ROLE == GAP_PERIPHERAL_ROLE)
static ptp_status_table PTPStatus [EXPECTED_CENTRAL_NODES]/*form the slaves*/
#endif
static app_service_t bleptp_service;
static app_attr_t bleptp_tx_att;
static app_attr_t bleptp_rx_att;
/**************************Service ID's*************************/

static const uint8_t ptp_service_uuid[16] = { 0x66, 0x9a, 0x0c,
		0x20, 0x00, 0x08, 0x96, 0x9e, 0xe2, 0x11, 0x9e, 0xb1, 0xf0, 0xf2, 0x73,
		0xd9 };
static const uint8_t ptp_TXchar_uuid[16] = { 0x66,0x9a,0x0c,
		0x20,0x00,0x08,0x96,0x9e,0xe2,0x11,0x9e,0xb1,0xe1,0xf2,0x73,
		0xd9};
static const uint8_t ptp_RXchar_uuid[16] = { 0x66,
	0x9a,0x0c,0x20,0x00,0x08,0x96,0x9e,0xe2,0x11,0x9e,0xb1,0xe2,0xf2,0x73,
	0xd9};
/***********************STATIC FUNC*********************/
static ptp_status_table * get_status_table(uint16_t Chandler);
static uint8_t ptp_packet_hdr_parse (uint8_t * data, uint8_t data_len, ptp_hdr * hdr);
static uint8_t create_ptp_packet_hdr(uint16_t chndler,uint8_t type, ptp_hdr * hdr,uint8_t *buff);
static uint8_t get_control_field(uint8_t type);
static uint8_t get_sequence_id( uint8_t type,uint16_t chndler);
static void input_packet_process( uint16_t chandler, 
                                  uint16_t attrhandler, 
                                  uint8_t data_length, 
                                  uint8_t *att_data, 
                                  tClockTime arval_time);
static void synchronization_process(ptp_status_table * st_sync_node);
static void ptp_error_handler(void);
static uint8_t ptp_enable_notify(uint16_t chandler);
static uint8_t ptp_disable_notify(uint16_t chandler);
static void sync_control_process_client(ptp_status_table * st_sync_node);
uint8_t ptp_send_ptp_packet(uint16_t chandler,
							uint8_t pkt_type,
							tClockTime * time_cpy, 
							tClockTime * time_delay);
static void clear_flags(void);
static void send_Cinterval_pkt_client(ptp_status_table * st_sync_node);
static void send_Cinterval_pkt_server(ptp_status_table * st_sync_node);





/**
* @brief  clean_buffer: clean the TX buffer .
  * @param uint8_t * buff : pointer to a buffer structure.
  * @param uint8_t length: size of the buffer.
  * param tClockTime * time_cpy> clock to send
  * @retval none
  */

static void clean_buffer(uint8_t * buff, uint8_t length)
{
  uint8_t i;
  for(i=0; i < length; i++){
    *buff++=0;
  }
}


/**
* @brief  clear_flags: Clean the time_synchronization Control Variables.
  * param none
  * @retval void.
  */


static void clear_flags()
{
  uint8_t i;
  for(i=0; i < max_number_entries; i++){
    if(PTPStatus[i].ptp_state != PTP_SYNCH){
      return;
    }
  }
  for(i=0; i < max_number_entries; i++){
    PTPStatus[i].ptp_state = PTP_FORWARD;
  }
  //printf("---------------------------------sync_success--------------------\n");
  sync_success=TRUE; 
  /*only if dynamic mode*/
  
#if defined (PTP_SERVER) 
#if !defined(USE_ONLY_PTP)
  if(ptp_mode==PTP_DYNAMIC)
  HAL_NVIC_SetPendingIRQ(PTP_NEW_SYNC_RESULT_SW_IRQn);/*SET A PENDING INTERRUPT TO THE CONTROL SERVICE TO UPDATE PARAMETERS*/
#endif
 /* enable_test_fun();*/
#endif
}


/**
* @brief  PTP_SET_operation_mode: 
  * This fuction defines the operation mode of the PTP-protocol
  * (PTP_STATIC:  This mode allows to operate Independently the PTP_service)
  * (PTP_DYNAMIC: This mode allows to operate the PTP_service in combination with the CTRL_service)
  * by default it's static i.e. PTP_STATIC
  * param none
  * @retval void.
  */

void PTP_SET_operation_mode(ptp_op_mode new_mode){
  ptp_mode = new_mode;
}



/**
* @brief  PTP_GET_status_tbl: send a ptp packet .
  * @param none
  * @retval ptp_status_table * : Current PTP_status TBL.
  */

ptp_status_table * PTP_GET_status_tbl(){
    return PTPStatus;
}


/**
* @brief  ptp_send_ptp_packet: This function Sends the respective PTP message to peer device.
  * @param uint16_t chandler: The connection handler associated to a peer device.
  * @param uint8_t pkt_type : The type of PTP packet.
  * @retval tClockTime * : Pointer to the peer clock structure.
  */

uint8_t ptp_send_ptp_packet(uint16_t chandler,uint8_t pkt_type,tClockTime * time_cpy, tClockTime * time_delay){
  tBleStatus res_ble;
  uint8_t ret;
  uint8_t tx_buffer[13];
  ptp_hdr rptp_hdr;
  
  ret = create_ptp_packet_hdr(chandler,pkt_type,&rptp_hdr,tx_buffer);
  if(ret==0)return ret;/*error*/
  
  if(pkt_type==SYNC)
  {
  *time_cpy = GET_CLOCK;
  }
   
  if (pkt_type == DELAY_REQ)
  {
    *time_cpy = GET_CLOCK;
    Osal_MemCpy(tx_buffer+ret+4,time_delay,sizeof(tClockTime)-1);
  }
  
  Osal_MemCpy(tx_buffer+ret,time_cpy,sizeof(tClockTime)-1);
    
 
  /*send the response to the ptp_client*/
  
  if(pkt_type!=DELAY_REQ)
  {
    res_ble = aci_gatt_write_without_response(chandler,bleptp_rx_att.Associate_CharHandler + 1, 13, tx_buffer);
  } 
  else{
    
    res_ble = aci_gatt_update_char_value(bleptp_service.ServiceHandle,bleptp_tx_att.CharHandle,0,13,tx_buffer);
  } 
  if(res_ble!= BLE_STATUS_SUCCESS)return 0;
 
  clean_buffer(tx_buffer,13);
  return ret;
  
}


/**
* @brief  ptp_enable_notify: enable the notify in the ptp_client.
  * @param uint16_t connHandler: the connection handler associated.
  * @retval 1 if success
  */



uint8_t ptp_enable_notify(uint16_t chandler){
  uint8_t notify_enable_data [] = {0x01,0x00};
  struct timer t;
  Timer_Set(&t, CLOCK_SECOND*10);
  
  while(aci_gatt_write_charac_descriptor(chandler,bleptp_tx_att.Associate_CharHandler+2,2,notify_enable_data)==BLE_STATUS_NOT_ALLOWED)
  {
    if(Timer_Expired(&t))return 0;/*error*/
  }
return 1;
}


/**
* @brief  ptp_disable_notify: desable the notify in the ptp_client.
  * @param uint16_t connHandler: the connection handler associated.
  * @retval 1 if success
  */

uint8_t ptp_disable_notify(uint16_t chandler){
uint8_t notify_disable_data [] = {0x00,0x00};
struct timer t;
Timer_Set(&t, CLOCK_SECOND*10);
while(aci_gatt_write_charac_descriptor(chandler,bleptp_tx_att.Associate_CharHandler+2,2,notify_disable_data)==BLE_STATUS_NOT_ALLOWED)
  {
    if(Timer_Expired(&t))return 0;/*error*/
  }

return 1;
}










/**
  * @brief  get_status_table:
  * This function return the PTP control table for a specific connection.
  * @param uint16_t connHandler: the connection handler associated.
  * @
  */

ptp_status_table * get_status_table(uint16_t Chandler){
  uint8_t i;
  ptp_status_table * rstable=NULL;
  
  for(i=0; i < max_number_entries; i++){
      if(PTPStatus[i].Chandler == Chandler)
		{
			rstable = &PTPStatus[i];
			break;
		}
      
  } 
  return rstable;
}


/**
  * @brief ptp_packet_hdr_parse
  * This function is able to parse the input ptp_packet.
  * @param uint8_t * data: packet itput.
  *	@param uint8_t data_len size of the packet.
  *	@param ptp_hdr * hdr : ptp_header structure 
  * @retval : payload pointer offset.
  */
uint8_t ptp_packet_hdr_parse (uint8_t * data, uint8_t data_len, ptp_hdr * hdr){
uint8_t * p;

	if(data_len < 5)
	{
		/*something is wrong return 0*/
		return 0;
	}	

	p=data;

		hdr->ptp_type =   (p[0] & 0xF);
        hdr->ptp_version= (p[0] >> 0x3) & 0x1;
        hdr->domain_number  = (p[0] >> 0x4) & 0x1;
        hdr->control_field = (p[0]>>0x5)& 0x1;
        hdr->sequence_id = p[1];
        hdr->msg_sync_interval = p[2];
        hdr->source_id = p[3] | ((uint16_t) p[4] << 0x8);
        
return PTP_HEADER_SIZE;
}


/**
  * @brief  This function creates a ptp packet header.
  * @param uint16_t chndler: destination peer connection handler.
  *	@param uint8_t  type: packet type.
  *	@param ptp_hdr * hdr : ptp_header data-structure
  *	@param uint8_t * buff : packet buffer
  * @retval : payload pointer.
  */

uint8_t create_ptp_packet_hdr(uint16_t chndler,uint8_t type, ptp_hdr * hdr,uint8_t *buff)
{
	uint8_t * p = buff;

	hdr->ptp_type=type;
	hdr->ptp_version = PTP_VERSION;
	hdr->domain_number = 0;
	hdr->control_field = get_control_field(type);
	hdr->sequence_id=get_sequence_id(type,chndler);
	hdr->msg_sync_interval = sync_interval;
	hdr->source_id = chndler;

	p[0] =  ((hdr->ptp_type & 0xF)) |
			((hdr->ptp_version & 0x1)<<0x3) |
			((hdr->domain_number & 0x1)<<0x4) |
			((hdr->control_field & 0x3)<<0x5);
	p[1] = hdr->sequence_id;
        
        p[2] = hdr->msg_sync_interval;
	
	p[3] = (hdr->source_id & 0xFF);

	p[4] = ((hdr->source_id >> 0x08)) & 0xFF; 

        return PTP_HEADER_SIZE; 		
}


/**
  * @brief  This function return the ptp control field value.
  *	@param uint8_t  type: packet type.
  * @retval : packet control field value.
  */

static uint8_t get_control_field(uint8_t type)
{
  uint8_t cfield=0;
  
  while(type & 0x1)
  {
    type = type >> 1;
    cfield+=1;
  }
  
  if(cfield > 3) ptp_error_handler();/*somwthing is wrong*/
  return cfield;
}


/**
  * @brief  This function return the sequence Id_field.
  *	@param uint8_t  type: packet type.
  *	@param uint8_t  chndler: connection handler associated.
  * @retval : packet control field value.
  */


static uint8_t get_sequence_id( uint8_t type,uint16_t chndler){
  
  ptp_status_table * st =  get_status_table(chndler);
  uint8_t rseq_number;
  
  rseq_number = st->seq_number;
  if(type==FOLLOW_UP || DELAY_RSP)
  {
    st->seq_number+=1;
  }
 
 return  rseq_number;
}

/**
  * @brief  This function update the virtual ptp_clock.
  *	@param ptp_clock_st * tm: ptp_ clock values.
  * @retval : void.
  */

static void ptp_update_clock(ptp_clock_st * tm){
 
    uint32_t pre_clock = GET_CLOCK;
    int32_t mp_delay = (((int32_t)tm->t1 - (int32_t)tm->t0) + ((int32_t)tm->t3 - (int32_t)tm->t2)) / 2; /*assuming constant delay*/
    int32_t offset = ((int32_t)tm->t1 - (int32_t)tm->t0) - (mp_delay);
    AJUST_CLOCK(offset);
#if PRINT_PTP    
    //printf("Timer_after: %d Delay: %d \n",clock_time(), mp_delay);
#endif 
    /*enable_test_fun();*/
   
}




/**
  * @brief  This function process the input syncronization Packages.
  * @param uint16_t chandler: 	 Associate connection Handler.
  * @param uint16_t attrhandler: Input attribute handler.
  * @param uint8_t data_length:  Length of the packet
  * @param tClockTime arval_time: Arraival time.
  */


static void input_packet_process( uint16_t chandler, 
                            uint16_t attrhandler, 
                            uint8_t data_length, 
                            uint8_t *att_data, 
                            tClockTime arval_time){

 uint8_t ret;
  ptp_hdr rptp_hdr;
  ptp_status_table * st;
  st = get_status_table(chandler);

  if(attrhandler==bleptp_tx_att.CharHandle + 2)
  {
     if(att_data[0] == 0x01){
      st->peer_ready_to_receive=TRUE;
     }else{
      st->peer_ready_to_receive=FALSE;
     }
  }else if (attrhandler == bleptp_rx_att.CharHandle + 1)
  {
    ret=ptp_packet_hdr_parse(att_data,data_length,&rptp_hdr);
    if(ret==0) ptp_error_handler();
    /*process the packet*/
    /*ptp_client had received 1. sync | 2. followup | 3. delay_resp*/
    if(rptp_hdr.ptp_type==SYNC && st->ptp_state==PTP_INIT){
      /*PROCESS SYNC PACKET*/
      sync_success=FALSE;
#if PRINT_PTP                                
BSP_ADD_LED_On(ADD_LED2);
#endif            
      st->timers.t1=arval_time;
#if PRINT_PTP                                
BSP_ADD_LED_Off(ADD_LED2);
#endif         
    }

    if(rptp_hdr.ptp_type == FOLLOW_UP && st->ptp_state==PTP_INIT){
      /*PROCESS FOLLOW_UP PACKET*/
#if PRINT_PTP                                
BSP_ADD_LED_On(ADD_LED2);
#endif

      Osal_MemCpy(&st->timers.t0,att_data+ret,sizeof(tClockTime)-1);
      st->pending_tx=1;
#if PRINT_PTP                                
BSP_ADD_LED_Off(ADD_LED2);
#endif      
    }
    if(rptp_hdr.ptp_type == DELAY_RSP && st->ptp_state==PTP_PENDING_RESP){
    /*PROCESS DELAY_RSP PACKET*/   
#if PRINT_PTP                                
BSP_ADD_LED_On(ADD_LED2);
#endif            

    Osal_MemCpy(&st->timers.t3,att_data+ret,sizeof(tClockTime)-1);
    ptp_update_clock(&(st->timers));
    sync_success=TRUE;
    st->ptp_state = PTP_INIT;

#if PRINT_PTP                                
BSP_ADD_LED_Off(ADD_LED2);
#endif   

  }
}


  if(attrhandler==bleptp_tx_att.Associate_CharHandler+1){
      ret = ptp_packet_hdr_parse(att_data, data_length, &rptp_hdr);
      if(ret==0) ptp_error_handler();

      if(st->ptp_state==PTP_WAIT_RESP && 
          rptp_hdr.ptp_type== DELAY_REQ){
        /*PROCESS DELAY_REQ PACKETS*/
#if PRINT_PTP                                
BSP_ADD_LED_On(ADD_LED2);
#endif               
                    //uint32_t pre_clock = clock_time();

  /*PENDING DELAY_RSP-TX*/
                    /*ret = ptp_send_ptp_packet(chandler,DELAY_RSP,&arval_time);*/
#if PRINT_PTP                                
BSP_ADD_LED_Off(ADD_LED2);
#endif                  
          st->timers.t2=arval_time;
          Osal_MemCpy(&st->timers.t3,att_data+ret,sizeof(tClockTime)-1);
          Osal_MemCpy(&st->timers.t1,att_data+ret+4,sizeof(tClockTime)-1);
          st->pending_tx=1;
          // clear_flags();  
                  
      }
  }
}

/**
  * @brief  This function initialize the ptp_service.
  * @ This function must be called at the begining of the application.
  * @param profile datastructure.
  * @
  */
void init_ptp_profile(app_profile_t * profile){
  APP_Status ret; 
/*create the ptp_service */
  COPY_VAR(bleptp_service.ServiceUUID,ptp_service_uuid);
  bleptp_service.service_uuid_type=UUID_TYPE_128;
  bleptp_service.service_type=SECONDARY_SERVICE;
  bleptp_service.max_attr_records=6;
  /*copy the and associate the service to the BLE application profile*/
#if defined (PTP_SERVER) 
  bleptp_service.serv_exposed = FALSE;
  bleptp_service.serv_to_disc = TRUE;
  
#elif defined (PTP_CLIENT) 
  bleptp_service.serv_exposed = TRUE;
  bleptp_service.serv_to_disc = FALSE;
#endif  
  
  
  ret = APP_add_BLE_Service(profile,&bleptp_service);
  
  if(ret!=APP_SUCCESS) ptp_error_handler();
  /*create the ptp_TX_attribute*/
  COPY_VAR(bleptp_tx_att.CharUUID,ptp_TXchar_uuid);
  bleptp_tx_att.charUuidType = UUID_TYPE_128;
  bleptp_tx_att.charValueLen = 13;
  bleptp_tx_att.charProperties = CHAR_PROP_NOTIFY;
  bleptp_tx_att.secPermissions = ATTR_PERMISSION_NONE;
  bleptp_tx_att.gattEvtMask = GATT_DONT_NOTIFY_EVENTS;
  bleptp_tx_att.encryKeySize=16;
  bleptp_tx_att.isVariable=1;
  /*copy and associate the ptp_TX_attribute to the ptp service*/
  
#if defined (PTP_SERVER) 
  bleptp_tx_att.char_exposed = FALSE;
  bleptp_tx_att.char_to_disc = TRUE;
#elif defined (PTP_CLIENT) 
  bleptp_tx_att.char_exposed = TRUE;
  bleptp_tx_att.char_to_disc = FALSE;
#endif  
  
  
   ret= APP_add_BLE_attr(&bleptp_service,&bleptp_tx_att);
    if(ret!=APP_SUCCESS)ptp_error_handler();
  /*create the ptp_RX_attribute*/
   COPY_VAR(bleptp_rx_att.CharUUID,ptp_RXchar_uuid);
  bleptp_rx_att.charUuidType = UUID_TYPE_128;
  bleptp_rx_att.charValueLen = 13;
  bleptp_rx_att.charProperties = CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RESP;
  bleptp_rx_att.secPermissions = ATTR_PERMISSION_NONE;
  bleptp_rx_att.gattEvtMask = GATT_NOTIFY_ATTRIBUTE_WRITE;
  bleptp_rx_att.encryKeySize=16;
  bleptp_rx_att.isVariable=1;
  
  
  #if defined (PTP_SERVER) 
  bleptp_rx_att.char_exposed = FALSE;
  bleptp_rx_att.char_to_disc = TRUE;
#elif defined (PTP_CLIENT) 
  bleptp_rx_att.char_exposed = TRUE;
  bleptp_rx_att.char_to_disc = FALSE;
#endif  
  /*copy and associate the RX_attribute to a service*/
  ret= APP_add_BLE_attr(&bleptp_service,&bleptp_rx_att); 
    if(ret!=APP_SUCCESS)ptp_error_handler();
    
 /*init PTP_clock*/
    //clock_interrupt_init(6);  /*do not initialize it here*/
}


/**
  * @brief  This function start the ptp protocol.
  * @param n_peers : number of sources  
  * @return : none
  */
void ptp_Start(uint8_t no_peers)
{
  max_number_entries =  no_peers;
  uint8_t i;
  ptp_status_table * ptr = PTPStatus;
    
 /*initialize the ptp_protocol_status and variables*/
CLOCK_RESET;  

  for (i=0; i<no_peers; i++ ){
      ptr->Chandler =  NET_get_chandler_by_index (i);
      ptr->node_id= i+1;
      ptr->ptp_state = PTP_INIT;
      ptr->seq_number=0;
      ptr->pending_tx=0;  
      ptr++;
  } 
 // if(NET_get_device_role() == DEVICE_CENTRAL) 
 /*init the virtual Clock*/ 
  #if defined (TEST_PTP_SERVER) 
  BlueNRG_ConnInterval_Init(10); /*this must be define by the top API*/
  #endif
  #if defined(PTP_SERVER)
    #if !defined(USE_ONLY_PTP)
  if(ptp_mode == PTP_DYNAMIC) ptp_to_ctrl_init(); /*INITIALIZES THE FEEDBACK INTERRUPT TO THE MEDIA SYNC SERVICE*/
    #endif
#endif
CLOCK_INIT;
   sync_success=TRUE;
}

/**
  * @brief  This function initialize the ptp protocol.
  * @param uint8_t ptp_dv_role: define if for this application the device 
  * will be (the master: who has the reference clock) or
  * (the slave: who synchonize its clock to a master reference clock).
  * @param  app_profile_t * profile: profile in where will be associated this application.
  * @
  */
void Init_ptp_application(uint8_t ptp_dv_role, app_profile_t * profile){
	uint8_t i;
	uint8_t expected_nodes;
	/*1. associate the role for this device*/
	ptp_role = ptp_dv_role;
	/*associate this service to the reference profile*/
	init_ptp_profile(profile);
	/*for each spected connection initialize the status connection*/

/*for each spected connection initialize the status connection*/
        if (NET_get_device_role() == DEVICE_CENTRAL){
	expected_nodes = EXPECTED_NODES;
        }else if(NET_get_device_role() == DEVICE_PERISPHERAL){
	expected_nodes = EXPECTED_CENTRAL_NODES;
        }

/*initialize the ptp_protocol_status and variables*/
CLOCK_RESET;
for(i=0; i <expected_nodes;i ++){	
	PTPStatus[i].ptp_state=PTP_INIT;
        PTPStatus[i].ptp_Substate=PTP_WSYNC;
        PTPStatus[i].seq_number=0;
        PTPStatus[i].pending_tx=0;
}

#if PRINT_PTP
BSP_ADD_LED_Init(ADD_LED2);
BSP_ADD_LED_Off(ADD_LED2);
	
#endif	          


/*INITIALIZE THE CONNECTION INTERVAL INTERRUPTION USED TO SEND DATA SYNCHRONOUSLY*/
//BlueNRG_ConnInterval_Init(10);
/*init the virtual Clock*/ 
CLOCK_INIT;
   
}



/**
  * @brief  This function handler the behavior of the ptp_server.
  * @param uint8_t ptp_dv_role: define if for this application the device 
  * will be (the master: who has the reference clock) or
  * (the slave: who synchonize its clock to a master reference clock).
  * @param  app_profile_t * profile: profile in where will be associated this application.
  * @
  */

void ptp_server_sync_process(){
	if(!network_get_status())
	{
		event_t * event;
  		event = (event_t *)HCI_Get_Event_CB();

  		if(event!=NULL && event->event_type == EVT_LE_CONN_COMPLETE){
  			        evt_le_connection_complete *cc = (void*)event->evt_data;
        			PTPStatus[max_number_entries].Chandler = cc->handle;
                                PTPStatus[max_number_entries].node_id = max_number_entries;
        			max_number_entries+=1;
  		}
  		/*return since we can't synchronize before to connection stablishment*/
  		return;

	}
	if (sync_success != TRUE){
		/*the ptp_server device needs to resynchronize*/
		event_t * event;
  		event = (event_t *)HCI_Get_Event_CB();

  		if(event!=NULL && event->event_type== EVT_BLUE_GATT_NOTIFICATION){
  			/*get the events associated to ptp protocol*/

  			evt_gatt_attr_notification *evt = (evt_gatt_attr_notification*)event->evt_data;
           			input_packet_process  (evt->conn_handle,
           									 evt->attr_handle,
           									 evt->event_data_length,
           									 evt->attr_value,
           									 event->ISR_timestamp
           									 );
                }  
        
      }

}


/**
  * @brief  ptp_server_sync_process_temp_new
	This function Replace the old ptp_server_sync_process.
  * @param none
  * @retval none
  */


void ptp_server_sync_process_temp_new(){
    
  if (sync_success != TRUE){
		/*the ptp_server device needs to resynchronize*/
		event_t * event;
  		event = (event_t *)HCI_Get_Event_CB();

  		if(event!=NULL && event->event_type== EVT_BLUE_GATT_NOTIFICATION){
  			/*get the events associated to ptp protocol*/

  			evt_gatt_attr_notification *evt = (evt_gatt_attr_notification*)event->evt_data;
           			input_packet_process  (evt->conn_handle,
           									 evt->attr_handle,
           									 evt->event_data_length,
           									 evt->attr_value,
           									 event->ISR_timestamp
           									 );
                }  
        
      }
}

/**
  * @brief  set_connection_clients: 
  *	associate a Chandler to and specific entry in the PTP_CTRL_TABLE.
  * @param uint8_t conn_entries: contain the number of devices connected as a clients.   
  * @return : none
  */
void set_connection_clients(uint8_t conn_entries)
{
  uint8_t i;
  max_number_entries = conn_entries;
  for (i=0; i < conn_entries; i++)
  {
      PTPStatus[i].Chandler = NET_get_chandler_by_index(i);
  }
}


/**
  * @brief  set_connection_servers: 
  *	associate a Chandler to and specific entry in the PTP_CTRL_TABLE.
  * @param uint8_t conn_entries: contain the number of devices connected as a server.   
  * @return : none
  */
void set_connection_servers(uint8_t serv_conn_entries)
{
   uint8_t i;
   max_number_entries = serv_conn_entries;
   for (i=0; i < serv_conn_entries; i++)
   {
      PTPStatus[i].Chandler = NET_get_chandler_by_index(i);
   }
    
}

/**
  * @brief  ptp_client_sync_process: 
  *	This is the main PTP process for a devices acting as client.
  * @return : none
  */
void ptp_client_sync_process(){
event_t * event;
event = (event_t *)HCI_Get_Event_CB();

  if(!network_get_status())
  {
    
      if(event!=NULL && event->event_type == EVT_LE_CONN_COMPLETE){
                evt_le_connection_complete *cc = (void*)event->evt_data;
              PTPStatus[max_number_entries].Chandler = cc->handle;
              max_number_entries+=1;
      }
      /*return since we can't synchronize before to connection stablishment*/
      return;

  }else if(network_get_status() && Cinterval_started ==0){
    Cinterval_started=1;
    BlueNRG_ConnInterval_IRQ_enable();
  }

  if(event!=NULL && event->event_type == EVT_BLUE_GATT_ATTRIBUTE_MODIFIED){

    uint8_t hw_version = get_harware_version();
        if(hw_version==IDB05A1){
          evt_gatt_attr_modified_IDB05A1 *evt = (evt_gatt_attr_modified_IDB05A1*)event->evt_data;
          input_packet_process (evt->conn_handle,
                      evt->attr_handle,
                      evt->data_length,
                      evt->att_data,
                      event->ISR_timestamp
                    );
        }

  }

   
                //if(PTPStatus[0].ptp_state != PTP_SYNCH){

}

/**
  * @brief  ptp_client_sync_process_new_tmp: 
  *	This Function replaces the old ptp_client_sync_process.
  * @return : none
  */
void ptp_client_sync_process_new_tmp()
{
  event_t * event;
  event = (event_t *)HCI_Get_Event_CB();
  
  
  
  if(event!=NULL && event->event_type == EVT_BLUE_GATT_ATTRIBUTE_MODIFIED){

    uint8_t hw_version = get_harware_version();
        if(hw_version==IDB05A1){
          evt_gatt_attr_modified_IDB05A1 *evt = (evt_gatt_attr_modified_IDB05A1*)event->evt_data;
          input_packet_process (evt->conn_handle,
                      evt->attr_handle,
                      evt->data_length,
                      evt->att_data,
                      event->ISR_timestamp
                    );
        }

  }
  
  
}


/**
  * @brief  send_Cinterval_pkt_client: This fuction handler properly the ptp packet transmission 
  *									   using the connection interval in devices acting as clients.
  * @param	ptp_status_table * : pointer to the PTP Synchonization table.
  * @return : none
  */

void send_Cinterval_pkt_client(ptp_status_table * st_sync_node){
uint8_t ret;
  if(sync_success!=TRUE && (st_sync_node->ptp_state == PTP_INIT) && (st_sync_node->pending_tx == 1)){

#if PRINT_PTP
BSP_ADD_LED_On(ADD_LED2);
#endif      

/*clear  PENDING DELAY-REQUEST-TX*/
    st_sync_node->pending_tx=0;
    ret = ptp_send_ptp_packet(st_sync_node->Chandler,DELAY_REQ,&st_sync_node->timers.t2,&st_sync_node->timers.t1);
    if(ret==0) ptp_error_handler();
    st_sync_node->ptp_state=PTP_PENDING_RESP;
    sync_success=TRUE;

#if PRINT_PTP
BSP_ADD_LED_Off(ADD_LED2);
#endif     

  }

}



/**
  * @brief  send_Cinterval_pkt_server: This fuction handler properly the ptp packet transmission 
  *									   using the connection interval in devices acting as servers.
  * @param	ptp_status_table * : pointer to the PTP Synchonization table.
  * @return : none
  */
void send_Cinterval_pkt_server(ptp_status_table * st_sync_node){
uint8_t ret;
  if(sync_success!=1){
    switch(st_sync_node->ptp_state){
      case PTP_INIT:
      {
        st_sync_node->local_notify_enable_flag=ptp_enable_notify(st_sync_node->Chandler);
        st_sync_node->ptp_state = PTP_FORWARD;
      }
      break;
      case PTP_FORWARD:
      {
#if PRINT_PTP
BSP_ADD_LED_On(ADD_LED2);
#endif  

        ret = ptp_send_ptp_packet(st_sync_node->Chandler,SYNC,&st_sync_node->timers.t0,NULL);
                                if(ret==0) ptp_error_handler();
#if PRINT_PTP
BSP_ADD_LED_Off(ADD_LED2);
#endif

#if PRINT_PTP
BSP_ADD_LED_On(ADD_LED2);
#endif  
        ret = ptp_send_ptp_packet (st_sync_node->Chandler,FOLLOW_UP,&st_sync_node->timers.t0,NULL);
                if(ret==0) ptp_error_handler();                              

#if PRINT_PTP
BSP_ADD_LED_Off(ADD_LED2);
#endif  
         st_sync_node->timers.t1= GET_CLOCK;
         st_sync_node->ptp_state=PTP_WAIT_RESP;                         
      }
      break;

      case PTP_WAIT_RESP:
      {
        if(st_sync_node->pending_tx==1)/*PENDING DELAY_RSP-TX*/
        {
          /*clear pendingdelay_response*/
#if PRINT_PTP
BSP_ADD_LED_On(ADD_LED2);
#endif           
           ret = ptp_send_ptp_packet(st_sync_node->Chandler,DELAY_RSP,&(st_sync_node->timers.t2),NULL);            
           if(ret==0) ptp_error_handler();  
#if PRINT_PTP
BSP_ADD_LED_Off(ADD_LED2);
#endif            
          /*clear pendingdelay_response*/
          st_sync_node->pending_tx=0;
           if(ret==0) ptp_error_handler();
           st_sync_node->ptp_state = PTP_SYNCH;

           
#if PRINT_PTP
BSP_ADD_LED_Off(ADD_LED2);
#endif            

#if PRINT_PTP                                                  
        //  printf( " ID: %d TIME: %d Delay: %d \n", st_sync_node->node_id ,clock_time() , (st_sync_node->timers.t2 - st_sync_node->timers.t1)/2);
#endif

        }
      }
      break;

    }

  }
}







/**
  * @brief  synchronization_process: 
  * This function forward the corresponding sync-follow msg to the slaves.
  * @param uint8_t ptp_status_table * st_sync_node : Control Structure For each_Node; 
  * will be (the master: who has the reference clock) or
  * (the slave: who synchonize its clock to a master reference clock).
  * @
  */

void synchronization_process(ptp_status_table * st_sync_node){
uint8_t ret;
	if(sync_success!=1){
		switch(st_sync_node->ptp_state){
			case PTP_INIT:
			{

				st_sync_node->local_notify_enable_flag=ptp_enable_notify(st_sync_node->Chandler);
				st_sync_node->ptp_state = PTP_FORWARD;
			}
			break;

			case PTP_FORWARD:
			{
#ifdef TEST_PTP
BSP_ADD_LED_On(ADD_LED2);
#endif                          
				ret = ptp_send_ptp_packet(st_sync_node->Chandler,SYNC,&st_sync_node->timers.t0,NULL);
                                if(ret==0) ptp_error_handler();
				ret = ptp_send_ptp_packet (st_sync_node->Chandler,FOLLOW_UP,&st_sync_node->timers.t0,NULL);
								if(ret==0) ptp_error_handler();                              
#if PRINT_PTP
BSP_ADD_LED_Off(ADD_LED2);
#endif                       
				
				 st_sync_node->timers.t1= GET_CLOCK;
				 Timer_Set(&(st_sync_node->packet_alive), PACKET_ALIVE_MS);/*set the timer used for restart the synchronization*/
				 st_sync_node->ptp_state=PTP_WAIT_RESP;               				
			}
			break;

			case PTP_WAIT_RESP:
			{
				if(Timer_Expired(&(st_sync_node->packet_alive)))
                                {
                                  
                                    
                                    st_sync_node->ptp_state=PTP_FORWARD;
                                    sync_success=FALSE;
                                }

			}
			break;

		}
	}
}




volatile uint8_t sync_id = 0;


/**
  * @brief  PTP_cinterval_IRQ_Handler 
  * This function handler the corresponding C-interval to a corresponding idx
  * @param uint32_t idx: connection interval index
  * @return: none.
  */
void PTP_cinterval_IRQ_Handler_idx(uint32_t idx)
{

#if Cinterval
BSP_ADD_LED_On(ADD_LED4);
BSP_ADD_LED_Off(ADD_LED4);
#endif   
  
#if defined (PTP_SERVER)
  send_Cinterval_pkt_server(&PTPStatus[idx]); 
  clear_flags();
#elif defined (PTP_CLIENT)
  send_Cinterval_pkt_client(&PTPStatus[idx]);
#endif  
  
}


/**
  * @brief  PTP_cinterval_IRQ_Handler This function handler the corresponding C-interval
  *	Interruption to forward the corresponding PTP messages
  * @return: none.
  */

void PTP_cinterval_IRQ_Handler(void)
{
/*  
#if Cinterval                                
BSP_ADD_LED_On(ADD_LED3);
BSP_ADD_LED_Off(ADD_LED3);
#endif 
*/
	if(network_get_status())
	{

		send_Cinterval_pkt_server(&PTPStatus[sync_id]);

#if defined(PTP_SERVER)		
		sync_id = !sync_id;
		clear_flags();
#elif defined(PTP_CLIENT)
		sync_id = 0;
#endif 

	}
}



static void sync_control_process_client(ptp_status_table * st_sync_node){
  
  if(Timer_Expired((&(st_sync_node->packet_alive))) && st_sync_node->ptp_Substate != PTP_WSYNC){
     st_sync_node->ptp_state = PTP_INIT;
     st_sync_node->ptp_Substate = PTP_WSYNC;
  }
}

/**
  * @brief  PTP_SYNC enable the control synchonization protocol
  *	Interruption to forward the corresponding PTP messages
  * @return: none.
  */
void PTP_SYNC (void){
sync_success=FALSE;
#if defined(TEST_PTP_SERVER)
//BlueNRG_ConnInterval_IRQ_enable();
#endif
}



/**
  * @brief  PTP_SYNC_set_periodic_sync: 
			This function set up a periodic Interruption 
  			to run the synchronization periodically.
  * @param  uint16_t period: Synchronization Period (ms).
  * @ return: none
  */

void PTP_SYNC_set_periodic_sync(uint32_t period){
	ptp_interrupt_init(period,6);
}

/**
  * @brief PTP_SYNC_enable_periodic_sync: 
  * This function enable a periodic synchronization interruption.
  * @ return: none
  */

void PTP_SYNC_enable_periodic_sync(void){
	ptp_interrupt_resume();
}

/**
  * @brief PTP_SYNC_desable_periodic_sync:
  * This function disable the periodic synchronization interruption.
  * @ return : none
  */
void PTP_SYNC_desable_periodic_sync(void){
	ptp_interrupt_suspend();
}

/**
  * @brief PTP_SYNC_update_periodic_sync:
  * This function  update the synchronization period.
  * @param uint32_t period: synchronization period
  * @return : none
  */
void PTP_SYNC_update_periodic_sync(uint32_t period){
	ptp_update_interrupt(period,6);
}

/**
  * @brief PTP_SYNC_switch_off_periodic_sync: 
  *	This function  deinit the Sync interruption.
  * @return : none
  */

void PTP_SYNC_switch_off_periodic_sync(void){
	void ptp_interrupt_deinit();
}


/**
  * @brief PTP_SYNC_IRQ_Handler:
  * This function  is called each time the SYNC interruption is raised.
  * @return : none
  */

void PTP_SYNC_IRQ_Handler(){
	sync_success=FALSE;
        if(!Cinterval_started){
        Cinterval_started = 1;
#if defined(TEST_PTP_SERVER)                          
        BlueNRG_ConnInterval_IRQ_enable();
#endif
      }        
}

/**
  * @brief PTP_Update_CTRL_Parameters_Callback: 
  * the synchonization Parameters at the Control Services.   
  * @return : none
  */
void PTP_Update_CTRL_Parameters_Callback(void){
  /**/
}



/**
  * @brief ptp_error_handler:
  * This function  used as a error handler.
  */
static void ptp_error_handler(void){
while(1);
}