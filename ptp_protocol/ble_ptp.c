/*Simple Sync-PTP BLE protocol*/

#ifdef BLE_APP_CONFIG
#include  <blefw_conf.h>
#endif

#ifdef PTP_TEST_WITH_IRQ
#include "ptp_time_int_test.h"
#endif

#include "ble_ptp.h"
#include "app_ble.h"
#include "ble_firmware.h"
#include "common_tables.h"
#include "clock.h"

#ifndef COPY_VAR
#define COPY_VAR(source,dest) (memcpy((source),(dest),sizeof(dest)))
#endif



/**************************Service Variables*************************/
static uint8_t ptp_role;
static uint8_t max_number_entries;
static uint8_t test_max=0;

static uint32_t TM [2][MAX_EXECUTIONS/2];
static uint8_t tm_index=0;
static uint8_t tm_index1=0;



#if (ROLE==GAP_CENTRAL_ROLE)
static ptp_status_table PTPStatus [EXPECTED_NODES];/*form the master */
#elif (ROLE == GAP_PERIPHERAL_ROLE)
static ptp_status_table PTPStatus [EXPECTED_CENTRAL_NODES]/*form the slaves*/
#endif
static app_service_t bleptp_service;
static app_attr_t bleptp_tx_att;
static app_attr_t bleptp_rx_att;

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
static uint8_t get_control_field(uint8_t type);
static uint8_t get_sequence_id(uint8_t type, uint16_t cHandler);
static ptp_status_table * get_status_table(uint16_t Chandler);
static uint8_t ptp_send_ptp_packet(uint16_t chandler,uint8_t pkt_type,tClockTime * time_cpy);
static uint8_t ptp_attribute_modified_CB(uint16_t chandler, uint16_t attrhandler, uint8_t data_length, uint8_t *att_data, tClockTime arval_time);
static void ptp_server_process(ptp_status_table * st_sync_node);
static uint8_t ptp_enable_notify(uint16_t chandler);
static uint8_t ptp_disable_notify(uint16_t chandler);
static void ptp_client_process(ptp_status_table * st_sync_node);
static void ptp_update_clock(ptp_clock_st * tm);
/*******************************************************************************/


/**
  * @brief  This function initialize the ptp_service.
  * @ This function must be called at the beginning of the application.
  * @param profile datastructure.
  * @
  */
static void init_ptp_profile(app_profile_t * profile){
  APP_Status ret; 
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
   COPY_VAR(bleptp_rx_att.CharUUID,ptp_RXchar_uuid);
  bleptp_rx_att.charUuidType = UUID_TYPE_128;
  bleptp_rx_att.charValueLen = 20;
  bleptp_rx_att.charProperties = CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RESP;
  bleptp_rx_att.secPermissions = ATTR_PERMISSION_NONE;
  bleptp_rx_att.gattEvtMask = GATT_NOTIFY_ATTRIBUTE_WRITE;
  bleptp_rx_att.encryKeySize=16;
  bleptp_rx_att.isVariable=1;
  /*copy and associate the RX_attribute to a service*/
  ret= APP_add_BLE_attr(&bleptp_service,&bleptp_rx_att); 
    if(ret!=APP_SUCCESS)ptp_error_handler();
}





void ptp_error_handler(void){
	while(1);
}


/**
  * @brief  This function return the specific PTP status per connection.
  * @param uint16_t connHandler: the connection handler associated to a connection status.
  * @
  */

static ptp_status_table * get_status_table(uint16_t Chandler){
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


static  uint8_t get_connection_index(uint16_t Chandler){
  uint8_t i;
 
  
    for(i=0; i < max_number_entries; i++)
    {
  
        if(PTPStatus[i].Chandler == Chandler)
          {
			return i;
          }
    
    }
  return 2;
}


static ptp_state_t * ptp_get_status(uint16_t connHandler){

/*since the number of entries are very small up to 8 
 *it is not necessary to implement a complex hash table to associate
 *the connection handler to the status, however this implementation 
 *must to pay a constant O(n) for search thought the entries.
 *it is recommended to implement a hash table to reduce this overhead
 *(in case in which it could be needed)*/
uint8_t i;
ptp_state_t * rptp_status = NULL;


	for(i=0; i < max_number_entries; i++)
	{
		if(PTPStatus[i].Chandler == connHandler)
		{
			rptp_status = &PTPStatus[i].ptp_state;
			break;
		}

	}
return  rptp_status;
}

/**
  * @brief  This function initialize the PTP protocol.
  * @param uint8_t ptp_dv_role: define if for this application the device 
  * will be (the master: who has the reference clock) or
  * (the slave: who synchronize its clock to a master reference clock).
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
		PTPStatus[i].ptp_state=PTP_INIT;
                PTPStatus[i].dv_state=PTP_UNSYNC;
                PTPStatus[i].seq_number=0;
	}

clock_reset();
#ifdef PTP_TEST_WITH_IRQ
ptp_interrupt_test_init(1);
ptp_Suspend_test_interrupt();
#endif
return PTP_SUCESSS;
}


uint8_t ptp_packet_hdr_parse(uint8_t * data, uint8_t data_len, ptp_hdr * hdr){
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
        
return 5;
}

uint8_t create_ptp_packet_hdr(uint16_t chndler,uint8_t type, ptp_hdr * hdr,uint8_t *buff)
{
	uint8_t * p = buff;

	hdr->ptp_type=type;
	hdr->ptp_version = PTP_VERSION;
	hdr->domain_number = 0;
	hdr->control_field = get_control_field(type);
	hdr->sequence_id=get_sequence_id(type,chndler);
	hdr->msg_sync_interval = SYNC_INTERVAL_MS;
	hdr->source_id = chndler;

	p[0] =  ((hdr->ptp_type & 0xF)) |
			((hdr->ptp_version & 0x1)<<0x3) |
			((hdr->domain_number & 0x1)<<0x4) |
			((hdr->control_field & 0x3)<<0x5);
	p[1] = hdr->sequence_id;
        
        p[2] = hdr->msg_sync_interval;
	
	p[3] = (hdr->source_id & 0xFF);

	p[4] = ((hdr->source_id >> 0x08)) & 0xFF; 

        return 5; 		
}

static uint8_t get_control_field(uint8_t type)
{
  uint8_t cfield=0;
  
  while(type & 0x1)
  {
    type = type >> 1;
    cfield+=1;
  }
  
  if(cfield > 3) ptp_error_handler();/*Something is wrong*/
  return cfield;
}


uint8_t get_sequence_id( uint8_t type,uint16_t chndler){
  
  ptp_status_table * st =  get_status_table(chndler);
  uint8_t rseq_number;
  
  rseq_number = st->seq_number;
  if(type==FOLLOW_UP || DELAY_RSP)
  {
    st->seq_number+=1;
  }
 
 return  rseq_number;
}

static void ptp_update_clock(ptp_clock_st * tm){
    int32_t mp_delay = ((tm->t1 - tm->t0) + (tm->t3 - tm->t2)) / 2; /*assuming constant delay*/
    int32_t offset = (int32_t)(tm->t1 - tm->t0) - (mp_delay);
    set_ticks(offset);
}

static uint8_t ptp_attribute_modified_CB(uint16_t chandler, 
                                  uint16_t attrhandler, 
                                  uint8_t data_length, 
                                  uint8_t *att_data, 
                                  tClockTime arval_time)
{
  uint8_t ret;
  ptp_hdr rptp_hdr;
  
  ptp_status_table * st = get_status_table(chandler);
  
    if(attrhandler==bleptp_tx_att.CharHandle + 2)
    {
      if(att_data[0] == 0x01){
        st->peer_ready_to_receive=TRUE;
      }else{
        st->peer_ready_to_receive=FALSE;
      }
    }else if(attrhandler==bleptp_rx_att.CharHandle + 1)
    {
      ret=ptp_packet_hdr_parse(att_data,data_length,&rptp_hdr);
      if (ret==0) return ret;/*an error occur*/
      /*ptp_client had received 1. sync | 2. followup | 3. delay_resp*/
      if(ptp_role==PTP_CLIENT)
      {
        switch(st->ptp_state)
        {
            case PTP_UNITIALIZED:
            {
              /*if this state ocour it is an error*/
            }
            break;
            case PTP_WAIT_RESP:
            {
              if(rptp_hdr.ptp_type==SYNC){
                st->timers.t1=arval_time;
                st->dv_state=PTP_UNSYNC;
              }
              
              if(rptp_hdr.ptp_type==FOLLOW_UP)
              {
                  Osal_MemCpy(&st->timers.t0,att_data+ret,sizeof(tClockTime));
                  ret = ptp_send_ptp_packet(st->Chandler,DELAY_REQ,&st->timers.t2);
                  if(ret==0)while(1);
              }

              if (rptp_hdr.ptp_type==DELAY_RSP)
              {
                Osal_MemCpy(&st->timers.t3,att_data+ret,sizeof(tClockTime));
                 ptp_update_clock(&(st->timers));
                 Timer_Set(&st->remain_sync_time, SYNC_INTERVAL_MS);/*set the synchonization interval*/
                 st->dv_state=PTP_SYNC;
              }
            }
            break;
        }
      }
      
    }
return 1;
}

uint8_t ptp_send_ptp_packet(uint16_t chandler,uint8_t pkt_type,tClockTime * time_cpy){
  tBleStatus res_ble;
  uint8_t ret;
  uint8_t tx_buffer[8];
  ptp_hdr rptp_hdr;
  
  ret = create_ptp_packet_hdr(chandler,pkt_type,&rptp_hdr,tx_buffer);
  if(ret==0)return ret;/*error*/
  
  if(pkt_type==SYNC)
  {
  *time_cpy = clock_time();
  }
   
  if (pkt_type == DELAY_REQ)
  {
    *time_cpy = clock_time();
  }
  
  Osal_MemCpy(tx_buffer+ret,time_cpy,sizeof(tClockTime));
  
 
  /*send the response to the ptp_client*/
  
  if(pkt_type!=DELAY_REQ)
  {
    res_ble = aci_gatt_write_without_response(chandler,bleptp_rx_att.Associate_CharHandler + 1, 8, tx_buffer);
  } 
  else{
    
    res_ble = aci_gatt_update_char_value(bleptp_service.ServiceHandle,bleptp_tx_att.CharHandle,0,8,tx_buffer);
  } 
  if(res_ble!= BLE_STATUS_SUCCESS)return 0;
 
  return ret;
  
}

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


uint8_t ptp_disable_notify(uint16_t chandler){
uint8_t notify_disable_data [] = {0x00,0x00};
struct timer t;
Timer_Set(&t, CLOCK_SECOND*10);
while(aci_gatt_write_charac_descriptor(chandler,bleptp_tx_att.Associate_CharHandler+2,2,notify_disable_data)==BLE_STATUS_NOT_ALLOWED)
  {
    if(Timer_Expired(&t))return 0;/*error*/
  }

return 0;
}


void GATT_GET_Notification_CB(uint16_t chandler, uint16_t attrhandler, uint8_t data_length, uint8_t *att_data, tClockTime arval_time)
{
  uint8_t ret;
   ptp_hdr rptp_hdr;
   uint8_t cidex;
   ptp_status_table * st;
    /* Evaluate if this attr_handler is associated to this process*/
    /* before to process the packet*/
   if(attrhandler!=bleptp_tx_att.Associate_CharHandler+1)return;
    st = get_status_table(chandler); 
    ret = ptp_packet_hdr_parse(att_data, data_length, &rptp_hdr);
    
    cidex = get_connection_index(chandler);
    if(ret==0)while(1);/*error occur*/
    
    if(st->dv_state==PTP_UNSYNC && 
       ptp_role==PTP_SERVER && 
         st->ptp_state==PTP_WAIT_RESP)
    {
      ptp_send_ptp_packet(chandler,DELAY_RSP,&arval_time);
          TM[cidex][tm_index]=arval_time;
          if(cidex==0)
          tm_index+=1;
          else
          tm_index1+=1;  
      
#ifdef PTP_TEST_WITH_IRQ
      ptp_Resume_test_interrupt();
#endif
       Timer_Set(&st->remain_sync_time, SYNC_INTERVAL_MS);/*set the synchonization interval*/
       st->dv_state=PTP_SYNC;  
    }
}







void ptp_service_process(){
  /*get_event :o*/
  uint8_t i;
  event_t * event;
  event = (event_t *)HCI_Get_Event_CB();
  if(event!=NULL){
    switch(event->event_type)
    {
      case EVT_LE_CONN_COMPLETE:
      {
        evt_le_connection_complete *cc = (void*)event->evt_data;
        PTPStatus[max_number_entries].Chandler = cc->handle;
        max_number_entries+=1;
      }
      break;
      
      case EVT_BLUE_GATT_ATTRIBUTE_MODIFIED:
      {
         uint8_t hw_version = get_harware_version();
         if(hw_version==IDB05A1){
         evt_gatt_attr_modified_IDB05A1 *evt = (evt_gatt_attr_modified_IDB05A1*)event->evt_data;
         ptp_attribute_modified_CB(evt->conn_handle,evt->attr_handle,evt->data_length,evt->att_data,event->ISR_timestamp);
         }
      }
      break;
      
      case EVT_BLUE_GATT_NOTIFICATION:
        {
           evt_gatt_attr_notification *evt = (evt_gatt_attr_notification*)event->evt_data;
           GATT_GET_Notification_CB(evt->conn_handle,evt->attr_handle,evt->event_data_length,evt->attr_value,event->ISR_timestamp);
        }
        break;
    }
  
  }
  
  if(network_get_status())
  {
    for(i=0; i < max_number_entries; i++)
    {
      
      if(ptp_role==PTP_SERVER) ptp_server_process(&PTPStatus[i]);
      if(ptp_role==PTP_CLIENT) ptp_client_process(&PTPStatus[i]);
    }
      
  }
  
  /*for each connection :o*/
 
}




static void stop(){
 ptp_Suspend_test_interrupt();  
BSP_LED_On(LED2);
while(1);
}



void ptp_server_process(ptp_status_table * st_sync_node)
{
    /*ptp server process*/
  uint8_t ret;

   switch(st_sync_node->dv_state){
    case PTP_UNSYNC:
    {
        switch(st_sync_node->ptp_state)
        {
           case PTP_INIT:
           {
            ret = ptp_send_ptp_packet(st_sync_node->Chandler,SYNC,&st_sync_node->timers.t0);
            if(ret==0)while(1);/*and error occur*/
            st_sync_node->local_notify_enable_flag=ptp_enable_notify(st_sync_node->Chandler);
            ret = ptp_send_ptp_packet (st_sync_node->Chandler,FOLLOW_UP,&st_sync_node->timers.t0);
            if(ret==0)while(1);/*and error occur*/
            Timer_Set(&(st_sync_node->packet_alive), PACKET_ALIVE_MS);/*set the timer used for restart the synchronization*/
            st_sync_node->ptp_state=PTP_WAIT_RESP;
           }
           break;
           case PTP_WAIT_RESP:
           {
              if(Timer_Expired(&(st_sync_node->packet_alive)))
              {
                if(st_sync_node->local_notify_enable_flag)st_sync_node->local_notify_enable_flag=ptp_disable_notify(st_sync_node->Chandler);
                st_sync_node->dv_state=PTP_UNSYNC;
                st_sync_node->ptp_state=PTP_INIT;

              }

           }
           break;
        }
        break;
        case PTP_SYNC:
         {
              if(Timer_Expired(&(st_sync_node->remain_sync_time))){
#ifdef PTP_TEST_WITH_IRQ  
         if(test_max==MAX_EXECUTIONS){
          stop_clock();  
         stop();
          }else{
          test_max+=1;
          }          
                
        ptp_Suspend_test_interrupt();
#endif             
                  if(st_sync_node->local_notify_enable_flag) st_sync_node->local_notify_enable_flag=ptp_disable_notify(st_sync_node->Chandler);
                  st_sync_node->dv_state=PTP_UNSYNC;
                  st_sync_node->ptp_state=PTP_INIT;
              }
           }
           break;
        }
    }
} 


void ptp_client_process(ptp_status_table * st_sync_node)
{
    /*ptp client process*/

  if(st_sync_node->dv_state ==PTP_SYNC && Timer_Expired(&(st_sync_node->remain_sync_time)))
  {
#ifdef PTP_TEST_WITH_IRQ        
        ptp_Suspend_test_interrupt();
#endif  

      st_sync_node->dv_state=PTP_UNSYNC;
      st_sync_node->ptp_state=PTP_WAIT_RESP;
  }

}

