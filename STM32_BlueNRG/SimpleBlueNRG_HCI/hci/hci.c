/**
  ******************************************************************************
  * @file    hci.c 
  * @author  AMG RF Application Team
  * @brief   Function for managing framework required for handling HCI interface.
  ******************************************************************************
  *
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  */ 

#include "hal_types.h"
#include "osal.h"
#include "ble_status.h"
#include "hal.h"
#include "hci_const.h"
#include "gp_timer.h"
#include <eventhandler.h>

#include "stm32_bluenrg_ble.h"
#ifdef EXTERN_CLK_INTERFACE
#include "clock_interface.h" 
#endif
    
    


#define HCI_LOG_ON 0

#if (HCI_LOG_ON == 1)
#define PRINTF_HCI(...) printf(__VA_ARGS__)
#else
#define PRINTF_HCI(...)
#endif



#define HCI_READ_PACKET_NUM_MAX 		 (8)
#ifndef MIN
#define MIN(a,b)            ((a) < (b) )? (a) : (b)
#endif
#ifndef MAX
#define MAX(a,b)            ((a) > (b) )? (a) : (b)
#endif

tListNode hciReadPktPool;
tListNode hciReadPktRxQueue;
tHciDataPacket ** hciReadPacket;
/* pool of hci read packets */
static tHciDataPacket     hciReadPacketBuffer[HCI_READ_PACKET_NUM_MAX];

static volatile uint8_t hci_timer_id;
static volatile uint8_t hci_timeout;

static event_t * ptr_event = NULL;
event_t ret_event;


static const uint8_t EVT_TABLE [] = {EVT_LE_META_EVENT,EVT_VENDOR}; /*principal event_table*/

static const uint16_t SUB_EVT_TABLE [] = {EVT_LE_CONN_COMPLETE,EVT_LE_ADVERTISING_REPORT,EVT_BLUE_GATT_PROCEDURE_COMPLETE
                                         ,EVT_BLUE_GATT_ATTRIBUTE_MODIFIED,EVT_BLUE_GATT_WRITE_PERMIT_REQ,EVT_BLUE_GATT_READ_PERMIT_REQ
                                         ,EVT_BLUE_ATT_READ_BY_GROUP_TYPE_RESP,EVT_BLUE_GATT_NOTIFICATION,EVT_BLUE_ATT_READ_BY_TYPE_RESP
                                         ,EVT_BLUE_ATT_FIND_BY_TYPE_VAL_RESP,EVT_BLUE_GATT_DISC_READ_CHAR_BY_UUID_RESP,EVT_BLUE_ATT_READ_RESP
                                         ,EVT_BLUE_GAP_DEVICE_FOUND
                                         };/*sub_event_table*/

/*prototypes of functions for the fast get event function*/
static void * return_disconnection_complete_event(hci_event_pckt * event_pckt);
static void * return_connection_complete_event(hci_event_pckt * event_pckt);
static void * return_advertisement_report_event(hci_event_pckt * event_pckt);
static void * return_procedure_complete_event(hci_event_pckt * event_pckt);
static void * return_procedure_attribute_modified_event(hci_event_pckt * event_pckt);
static void * return_procedure_gatt_notifcation_event(hci_event_pckt * event_pckt);
static void * return_service_found_event(hci_event_pckt * event_pckt);
static void * return_attr_discovery_event(hci_event_pckt * event_pckt);
static void * return_attr_found_event(hci_event_pckt * event_pckt);
static void * return_default(hci_event_pckt * event_pckt);
static void * HCI_Process_event(tHciDataPacket * hciReadPacket);

 void * return_disconnection_complete_event(hci_event_pckt * event_pckt){
    return NULL;
}

void * return_default(hci_event_pckt * event_pckt){
   return NULL;
}

void * return_connection_complete_event(hci_event_pckt * event_pckt){
  return NULL;
}

void * return_advertisement_report_event(hci_event_pckt * event_pckt){
  return NULL;
}

void * return_procedure_complete_event(hci_event_pckt * event_pckt){
  return NULL;
}

void * return_procedure_attribute_modified_event(hci_event_pckt * event_pckt){
    return NULL; 
}

void * return_procedure_gatt_notifcation_event(hci_event_pckt * event_pckt){
  return NULL;
}


void * return_service_found_event(hci_event_pckt * event_pckt){
    return NULL;
}


void * return_attr_discovery_event(hci_event_pckt * event_pckt)
{
    return NULL;
}

void * return_attr_found_event(hci_event_pckt * event_pckt)
{
    return NULL;
}

uint16_t * index_of(const uint16_t * src, uint16_t * cmp_val, size_t range){
  uint16_t * ret_val = NULL;
  uint8_t i;
  uint16_t * ptr_src = (uint16_t *)src;
  
  for(i=0; i < range; i++)
  {
    uint8_t val = *ptr_src;
    if(val == *cmp_val)
    {
      ret_val = ptr_src;
      break;
    } 
    ptr_src++;
  }
  
  return ret_val;
}



void * HCI_GET_EVENT_FAST_CB(void){
  
   tHciDataPacket * hciReadPacket = NULL;
   event_t * ptr_event=NULL;
   uint16_t * cmdptr;
   uint8_t offset_evt;
   uint8_t offset_subevt;
   
  static void * (* const event_func_ptr[sizeof(EVT_TABLE)][sizeof(SUB_EVT_TABLE)])(hci_event_pckt *) = 
  { 
    { return_connection_complete_event,
      return_advertisement_report_event,
      return_default,return_default,
      return_default,return_default,
      return_default,return_default,
      return_default,return_default,
      return_default,return_default,
      return_default
    },
    
    {   return_default,return_default,
        return_procedure_complete_event,
        return_procedure_attribute_modified_event,
        return_default,return_default,return_default,
        return_procedure_gatt_notifcation_event,
        return_service_found_event,
        return_attr_discovery_event,
        return_attr_found_event,
        return_default,return_default
    }
  };
  
  
  
   Disable_SPI_IRQ();
   uint8_t list_empty = list_is_empty(&hciReadPktRxQueue);
  
     if(list_empty==FALSE)
   {
      list_get_head (&hciReadPktRxQueue, (tListNode **)&hciReadPacket);/**/
      Enable_SPI_IRQ();
      hci_uart_pckt * hci_pckt = ( hci_uart_pckt *)(hciReadPacket->dataBuff);
      hci_event_pckt * event_pckt = (hci_event_pckt*)(hci_pckt->data);
       
       if(hci_pckt->type != HCI_EVENT_PKT){
        /*BLUENRG only support event_packets*/
        return NULL;
        }
       
       if(event_pckt->evt==EVT_DISCONN_COMPLETE)
       {        
          return_disconnection_complete_event(event_pckt);
       }else
       {
        cmdptr = index_of((const uint16_t *)EVT_TABLE,(uint16_t *)(event_pckt->evt),sizeof(EVT_TABLE));
        
          if(cmdptr!=NULL)
          {
            cmdptr=NULL;
            offset_evt = (cmdptr - (uint16_t *)EVT_TABLE);
            if(offset_evt==0){
               evt_le_meta_event *evt = (void *)event_pckt->data;
               cmdptr = index_of (SUB_EVT_TABLE,(uint16_t *)(evt->subevent),sizeof(SUB_EVT_TABLE));
               
               
            }else if (offset_evt==1){
               evt_blue_aci * blue_evt = (void*)event_pckt->data;
                cmdptr = index_of (SUB_EVT_TABLE,(uint16_t *)(blue_evt->ecode),sizeof(SUB_EVT_TABLE));
            }
            if(cmdptr!=NULL){
              offset_subevt = (cmdptr - SUB_EVT_TABLE);
              ptr_event = (event_t *)(*event_func_ptr[offset_evt][offset_subevt])(event_pckt);
            }
            
          }
       
       }   
   }
return ptr_event;
}



void hci_timeout_callback(void)
{
  hci_timeout = 1;
  return;
}

void HCI_Init(void)
{
  uint8_t index;
  
  /* Initialize list heads of ready and free hci data packet queues */
  list_init_head (&hciReadPktPool);
  list_init_head (&hciReadPktRxQueue);
  
  /* Initialize the queue of free hci data packets */
  for (index = 0; index < HCI_READ_PACKET_NUM_MAX; index++)
  {
    list_insert_tail(&hciReadPktPool, (tListNode *)&hciReadPacketBuffer[index]);
  }
  
  
}

#define HCI_PCK_TYPE_OFFSET                 0
#define  EVENT_PARAMETER_TOT_LEN_OFFSET     2

/**
 * Verify if HCI packet is correctly formatted.
 *
 * @param[in] hciReadPacket    The packet that is received from HCI interface.
 * @return 0 if HCI packet is as expected
 */
int HCI_verify(const tHciDataPacket * hciReadPacket)
{
  const uint8_t *hci_pckt = hciReadPacket->dataBuff;
  
  if(hci_pckt[HCI_PCK_TYPE_OFFSET] != HCI_EVENT_PKT)
    return 1;  /* Incorrect type. */
  
  if(hci_pckt[EVENT_PARAMETER_TOT_LEN_OFFSET] != hciReadPacket->data_len - (1+HCI_EVENT_HDR_SIZE))
    return 2; /* Wrong length (packet truncated or too long). */
  
  return 0;      
}

void HCI_Process(void)
{
  tHciDataPacket * hciReadPacket = NULL;
  
  Disable_SPI_IRQ();
  uint8_t list_empty = list_is_empty(&hciReadPktRxQueue);        
  /* process any pending events read */
  while(list_empty == FALSE)
  {
    list_remove_head (&hciReadPktRxQueue, (tListNode **)&hciReadPacket);
    Enable_SPI_IRQ();
    HCI_Event_CB(hciReadPacket->dataBuff);
    Disable_SPI_IRQ();
    list_insert_tail(&hciReadPktPool, (tListNode *)hciReadPacket);
    list_empty = list_is_empty(&hciReadPktRxQueue);
  }
  /* Explicit call to HCI_Isr(), since it cannot be called by ISR if IRQ is kept high by
  BlueNRG. */
  HCI_Isr_Event_Handler_CB();
  Enable_SPI_IRQ();    
}

BOOL HCI_Queue_Empty(void)
{
  return list_is_empty(&hciReadPktRxQueue);
}


uint8_t Packet_Get_Priority(){
  return 0;
}

void HCI_Isr_Event_Handler_CB(){
	  /*Here had been modified the HCI_Isr method to allows:
  *
  *1. timestamp for all imput packages.
  *2. allocate the input packets into the packet queue with priorities.
  *
  */

  tHciDataPacket * hciReadPacket = NULL;
  uint8_t data_len;
  tClockTime HCI_Isr_time = GET_CLOCK; /*this is the current time in which the packet was transfered from the PoolQueue to the ReadRXqueue*/
  
  Clear_SPI_EXTI_Flag();
  while(BlueNRG_DataPresent()){        
    if (list_is_empty (&hciReadPktPool) == FALSE){
      
      /* enqueueing a packet for read */
      list_remove_head (&hciReadPktPool, (tListNode **)&hciReadPacket);
      
      data_len = BlueNRG_SPI_Read_All(hciReadPacket->dataBuff, HCI_READ_PACKET_SIZE);
      if(data_len > 0){                    
        hciReadPacket->data_len = data_len;
        if(HCI_verify(hciReadPacket) == 0){
           hciReadPacket->Isr_timestamp=HCI_Isr_time;	
          list_insert_tail(&hciReadPktRxQueue, (tListNode *)hciReadPacket);
        }else{
           hciReadPacket->Isr_timestamp=0;	
          list_insert_head(&hciReadPktPool, (tListNode *)hciReadPacket);          
        }
      }
      else {
        // Insert the packet back into the pool.
         hciReadPacket->Isr_timestamp = 0;
        list_insert_head(&hciReadPktPool, (tListNode *)hciReadPacket);
      }
      
    }
    else{
      // HCI Read Packet Pool is empty, wait for a free packet.
      Clear_SPI_EXTI_Flag();
      return;
    }
    
    Clear_SPI_EXTI_Flag();
  }
	
}





void * HCI_Get_Event_CB()
{
	tHciDataPacket * hciReadPacket = NULL;
  	
  	if(ptr_event!= NULL)
  	{
  		return (void*)ptr_event;
  	}


 Disable_SPI_IRQ();
 uint8_t list_empty = list_is_empty(&hciReadPktRxQueue);

	if(list_empty == FALSE)
	{
		list_get_head(&hciReadPktRxQueue, (tListNode **)&hciReadPacket);
		Enable_SPI_IRQ();
		ptr_event = HCI_Process_event(hciReadPacket); 

	}
return (void*)ptr_event;
}



static void * HCI_Process_event(tHciDataPacket * hciReadPacket)
{
   	   
        event_t * tmp_ptr_event=NULL;
	hci_uart_pckt * hci_pckt = ( hci_uart_pckt *)(hciReadPacket->dataBuff);
	hci_event_pckt * event_pckt = (hci_event_pckt*)(hci_pckt->data);

	 if(hci_pckt->type != HCI_EVENT_PKT){
        /*BLUENRG only support event_packets*/
        return NULL;
        }

    switch(event_pckt->evt){

    	case EVT_DISCONN_COMPLETE:
    	{

    		while(1); /*FIXME:  implementation leaft as a feature*/
    	}
    	break;
        

    	case EVT_LE_META_EVENT:
    	{
    		evt_le_meta_event *evt = (void *)event_pckt->data;
    		switch(evt->subevent)
    		{
    			case EVT_LE_CONN_COMPLETE:
    			{
    				evt_le_connection_complete *cc = (void *)evt->data;
                 	ret_event.event_type = EVT_LE_CONN_COMPLETE;
                 	ret_event.evt_data =cc;
                 	ret_event.ISR_timestamp = hciReadPacket->Isr_timestamp;
                 	tmp_ptr_event = &ret_event;
    			}
    			break;

    			case EVT_LE_ADVERTISING_REPORT:
    			{
                        le_advertising_info *pr = (le_advertising_info*) (((uint8_t*)evt->data)+1);
                 	ret_event.event_type = EVT_LE_ADVERTISING_REPORT;
                 	ret_event.evt_data = pr;
                 	ret_event.ISR_timestamp = hciReadPacket->Isr_timestamp;
                 	tmp_ptr_event = &ret_event;
    			}
    			break;

			  default:
              break;
    		}
    	}
    	break;
    	case EVT_VENDOR:
    	{
    		evt_blue_aci * blue_evt = (void*)event_pckt->data;
    		switch(blue_evt->ecode)
              {
              		case EVT_BLUE_GATT_PROCEDURE_COMPLETE:
              		{
              			evt_gatt_procedure_complete * pr = (void*)blue_evt->data;
                  		ret_event.event_type=EVT_BLUE_GATT_PROCEDURE_COMPLETE;
                  		ret_event.evt_data = pr;
                  		ret_event.ISR_timestamp = hciReadPacket->Isr_timestamp;
                  		tmp_ptr_event = &ret_event;
              		}
              		break;

              		case EVT_BLUE_GATT_ATTRIBUTE_MODIFIED:
              		{
                   		uint8_t hw_version = get_harware_version();
                   		if(hw_version==IDB05A1){
                    	evt_gatt_attr_modified_IDB05A1 *evt = (evt_gatt_attr_modified_IDB05A1*)blue_evt->data;
                    	ret_event.evt_data=evt;
                    	}else{
                    	evt_gatt_attr_modified_IDB04A1 * evt = (evt_gatt_attr_modified_IDB04A1*)blue_evt->data;
                    	ret_event.evt_data=evt;
                    	}

          	            ret_event.event_type = EVT_BLUE_GATT_ATTRIBUTE_MODIFIED;
          		        ret_event.ISR_timestamp=hciReadPacket->Isr_timestamp;
         	            tmp_ptr_event = &ret_event;
              		}
              		break;
              		case EVT_BLUE_GATT_WRITE_PERMIT_REQ:
              		{
              			while(1);/*FIXME:  implementation leaft as a feature*/
              		}
              		break;
              		case EVT_BLUE_GATT_READ_PERMIT_REQ:
              		{
              			while(1);/*FIXME:  implementation leaft as a feature*/
              		}
              		break;
              		case EVT_BLUE_ATT_READ_BY_GROUP_TYPE_RESP:
              		{
              			while(1);/*FIXME:  implementation leaft as a feature*/
              		}
              		break;
              		case EVT_BLUE_GATT_NOTIFICATION:
              		{

              			evt_gatt_attr_notification *evt = (evt_gatt_attr_notification*)blue_evt->data;
                  		ret_event.event_type=EVT_BLUE_GATT_NOTIFICATION;
                  		ret_event.evt_data = evt;
                  		ret_event.ISR_timestamp=hciReadPacket->Isr_timestamp;
                   		tmp_ptr_event = &ret_event;
              		}
              		break;
              		case EVT_BLUE_ATT_READ_BY_TYPE_RESP:
              		{
              			while(1);/*FIXME:  implementation leaft as a feature*/
              		}
              		break;
              		case EVT_BLUE_ATT_FIND_BY_TYPE_VAL_RESP:
              		{
              		    evt_att_find_by_type_val_resp * resp = ( evt_att_find_by_type_val_resp *)blue_evt->data;
	                    ret_event.event_type=EVT_BLUE_ATT_FIND_BY_TYPE_VAL_RESP;
   	                    ret_event.evt_data = resp;
      	                ret_event.ISR_timestamp=hciReadPacket->Isr_timestamp;
                        tmp_ptr_event = &ret_event;
              		}
              		break;
              		case EVT_BLUE_GATT_DISC_READ_CHAR_BY_UUID_RESP:
              		{
              	      evt_gatt_disc_read_char_by_uuid_resp * resp = (void *)blue_evt->data;
	                  ret_event.event_type=EVT_BLUE_GATT_DISC_READ_CHAR_BY_UUID_RESP;
    	              ret_event.evt_data=resp;
        	          ret_event.ISR_timestamp=hciReadPacket->Isr_timestamp;
            	      tmp_ptr_event = &ret_event;

              		}	
              		break;

              		case EVT_BLUE_ATT_READ_RESP:
              		{
              			while(1);/*FIXME:  implementation leaft as a feature*/
              		}
              		break;

              		case EVT_BLUE_GAP_DEVICE_FOUND:
              		{
              			/*case of IDB04A1*/
              		}
              		break;
                        case EVT_BLUE_GATT_TX_POOL_AVAILABLE:
                        {
                          //while(1); 
                          /*FIXME: This is raise when we excede the 
                                       max num of pkt tx supported by BLE*/
                        }
                        break;
              		default:
              		break;
              }
    	}
    	break;
    	default:
    	break;

    }
return (void*)tmp_ptr_event;        
}


void HCI_Packet_Release_Event_CB()
{

	tHciDataPacket * hciReadPacket = NULL;
	Disable_SPI_IRQ();
	uint8_t list_empty = list_is_empty(&hciReadPktRxQueue);
	ptr_event = NULL;
	if(list_empty == FALSE){
              list_remove_head (&hciReadPktRxQueue, (tListNode **)&hciReadPacket);
              list_insert_tail(&hciReadPktPool, (tListNode *)hciReadPacket);        
	}else{
		HCI_Isr_Event_Handler_CB();
		Enable_SPI_IRQ();
	}

}

/*void HCI_Packet_Release_Event_CB(){
tHciDataPacket * hciReadPacket = NULL;
Disable_SPI_IRQ();
list_remove_head (&hciReadPktRxQueue, (tListNode **)&hciReadPacket);
list_insert_tail(&hciReadPktPool, (tListNode *)hciReadPacket);
uint8_t list_empty = list_is_empty(&hciReadPktRxQueue);  
Enable_SPI_IRQ();    
}*/


  

void HCI_Isr(void)
{
  tHciDataPacket * hciReadPacket = NULL;
  uint8_t data_len;
  
  Clear_SPI_EXTI_Flag();
  while(BlueNRG_DataPresent()){        
    if (list_is_empty (&hciReadPktPool) == FALSE){
      
      /* enqueueing a packet for read */
      list_remove_head (&hciReadPktPool, (tListNode **)&hciReadPacket);
      
      data_len = BlueNRG_SPI_Read_All(hciReadPacket->dataBuff, HCI_READ_PACKET_SIZE);
      if(data_len > 0){                    
        hciReadPacket->data_len = data_len;
        if(HCI_verify(hciReadPacket) == 0)
          list_insert_tail(&hciReadPktRxQueue, (tListNode *)hciReadPacket);
        else
          list_insert_head(&hciReadPktPool, (tListNode *)hciReadPacket);          
      }
      else {
        // Insert the packet back into the pool.
        list_insert_head(&hciReadPktPool, (tListNode *)hciReadPacket);
      }
      
    }
    else{
      // HCI Read Packet Pool is empty, wait for a free packet.
      Clear_SPI_EXTI_Flag();
      return;
    }
    
    Clear_SPI_EXTI_Flag();
  }
}

void hci_write(const void* data1, const void* data2, uint8_t n_bytes1, uint8_t n_bytes2){
#if  HCI_LOG_ON
  PRINTF_HCI("HCI <- ");
  for(int i=0; i < n_bytes1; i++)
    PRINTF_HCI("%02X ", *((uint8_t*)data1 + i));
  for(int i=0; i < n_bytes2; i++)
    PRINTF_HCI("%02X ", *((uint8_t*)data2 + i));
  PRINTF_HCI("\n");    
#endif
  
  Hal_Write_Serial(data1, data2, n_bytes1, n_bytes2);
}

void hci_send_cmd(uint16_t ogf, uint16_t ocf, uint8_t plen, void *param)
{
  hci_command_hdr hc;
  
  hc.opcode = htobs(cmd_opcode_pack(ogf, ocf));
  hc.plen= plen;
  
  uint8_t header[HCI_HDR_SIZE + HCI_COMMAND_HDR_SIZE];
  header[0] = HCI_COMMAND_PKT;
  Osal_MemCpy(header+1, &hc, sizeof(hc));
  
  hci_write(header, param, sizeof(header), plen);
}

static void move_list(tListNode * dest_list, tListNode * src_list)
{
  pListNode tmp_node;
  
  while(!list_is_empty(src_list)){
    list_remove_tail(src_list, &tmp_node);
    list_insert_head(dest_list, tmp_node);
  }
}

 /* It ensures that we have at least half of the free buffers in the pool. */
static void free_event_list(void)
{
  tHciDataPacket * pckt;
    
  Disable_SPI_IRQ();
  
  while(list_get_size(&hciReadPktPool) < HCI_READ_PACKET_NUM_MAX/2){
    list_remove_head(&hciReadPktRxQueue, (tListNode **)&pckt);    
    list_insert_tail(&hciReadPktPool, (tListNode *)pckt);
    /* Explicit call to HCI_Isr(), since it cannot be called by ISR if IRQ is kept high by
    BlueNRG */
    HCI_Isr_Event_Handler_CB();//HCI_Isr();
  }
  
  Enable_SPI_IRQ();
}

int hci_send_req(struct hci_request *r, BOOL async)
{
  uint8_t *ptr;
  uint16_t opcode = htobs(cmd_opcode_pack(r->ogf, r->ocf));
  hci_event_pckt *event_pckt;
  hci_uart_pckt *hci_hdr;
  int to = DEFAULT_TIMEOUT;
  struct timer t;
  tHciDataPacket * hciReadPacket = NULL;
  tListNode hciTempQueue;
  
  list_init_head(&hciTempQueue);

  free_event_list();
  
  hci_send_cmd(r->ogf, r->ocf, r->clen, r->cparam);
  
  if(async){
    return 0;
  }
  
  /* Minimum timeout is 1. */
  if(to == 0)
    to = 1;
  
  Timer_Set(&t, to);
  
  while(1) {
    evt_cmd_complete *cc;
    evt_cmd_status *cs;
    evt_le_meta_event *me;
    int len;
      
    while(1){
      if(Timer_Expired(&t)){
        goto failed;
      }
      if(!HCI_Queue_Empty()){
        break;
      }
    }
    
    /* Extract packet from HCI event queue. */
    Disable_SPI_IRQ();
    list_remove_head(&hciReadPktRxQueue, (tListNode **)&hciReadPacket);    
    
    hci_hdr = (void *)hciReadPacket->dataBuff;

    if(hci_hdr->type == HCI_EVENT_PKT){
    
    event_pckt = (void *) (hci_hdr->data);
    
    ptr = hciReadPacket->dataBuff + (1 + HCI_EVENT_HDR_SIZE);
    len = hciReadPacket->data_len - (1 + HCI_EVENT_HDR_SIZE);
    
    switch (event_pckt->evt) {
      
    case EVT_CMD_STATUS:
      cs = (void *) ptr;
      
      if (cs->opcode != opcode)
        goto failed;
      
      if (r->event != EVT_CMD_STATUS) {
        if (cs->status) {
          goto failed;
        }
        break;
      }
      
      r->rlen = MIN(len, r->rlen);
      Osal_MemCpy(r->rparam, ptr, r->rlen);
      goto done;
      
    case EVT_CMD_COMPLETE:
      cc = (void *) ptr;
      
      if (cc->opcode != opcode)
        goto failed;
      
      ptr += EVT_CMD_COMPLETE_SIZE;
      len -= EVT_CMD_COMPLETE_SIZE;
      
      r->rlen = MIN(len, r->rlen);
      Osal_MemCpy(r->rparam, ptr, r->rlen);
      goto done;
      
    case EVT_LE_META_EVENT:
      me = (void *) ptr;
      
      if (me->subevent != r->event)
        break;
      
      len -= 1;
      r->rlen = MIN(len, r->rlen);
      Osal_MemCpy(r->rparam, me->data, r->rlen);
      goto done;
      
    case EVT_HARDWARE_ERROR:            
      goto failed;
      
    default:      
      break;
      }
    }
    
    /* If there are no more packets to be processed, be sure there is at list one
       packet in the pool to process the expected event.
       If no free packets are available, discard the processed event and insert it
       into the pool. */
    if(list_is_empty(&hciReadPktPool) && list_is_empty(&hciReadPktRxQueue)){
      list_insert_tail(&hciReadPktPool, (tListNode *)hciReadPacket);
      hciReadPacket=NULL;
    }
    else {
      /* Insert the packet in a different queue. These packets will be
      inserted back in the main queue just before exiting from send_req(), so that
      these events can be processed by the application.
    */
    list_insert_tail(&hciTempQueue, (tListNode *)hciReadPacket);
      hciReadPacket=NULL;
    }

    HCI_Isr_Event_Handler_CB();//HCI_Isr();
    
    Enable_SPI_IRQ();
    
  }
  
failed: 
  if(hciReadPacket!=NULL){
    list_insert_head(&hciReadPktPool, (tListNode *)hciReadPacket);
  }
  move_list(&hciReadPktRxQueue, &hciTempQueue);  
  Enable_SPI_IRQ();
  return -1;
  
done:
  // Insert the packet back into the pool.
  list_insert_head(&hciReadPktPool, (tListNode *)hciReadPacket); 
  move_list(&hciReadPktRxQueue, &hciTempQueue);
  
  Enable_SPI_IRQ();
  return 0;
}

