/*sync_control_interface*/
/*
* this interface is used to inform and provide synchronization parameters for BLE applications
* this could take advantage of the ptp_ synchronization mechanism development in a previous release
* this interface may handler multiple sink and source peer devices.
*/




/* Operation modes:
*
* a) using the PTP protocol for clock synchronization and time-stamp solution
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


/*common case master two slaves*/

/*

+--------+ Init()+--------+Sync()+--------+      +--------+     +--------+Init()+-------+
|        +------>+        +------>        <------+        <-----+        <------+       |
|        |get_   |        |      |        |      |        |     |        |Get   |       |
|API     |param()|CTRL    |      |PTP     |      |PTP     |     |CTRL    |Param()API    |
|Server  +------>+Server  |      |Server  |      |Client  |     |Client  <------+       |
|        |send   |        |notify|        |      |        |Notify        |Send  |       |
|        |param()|        |param()        |      |        |Param()       |param()       |
|        <-------+        <------+        +------>        +----->        +------>       |
+--------+       ++--^--^-+      +--+-----+      +--------+     +----^---+      +-------+
                  |  |  |           |                                |
                  |  |  +--------------------------------------------+
                  |  |              |
                  |  |              |
                  |  | Sync()       |
                  |  |              |
                  |  |              |               +--------+     +--------+Init()+-------+
                  |  |              +--------------->        <-----+        <------+       |
                  |  |                              |        |     |        |Get   |       |
                  |  |                              |PTP     |     |CTRL    |Param()API    |
                  |  |                              |Client  |     |Client  <------+       |
                  |  |                              |        |Notify        |Send  |       |
                  |  |                              |        |Param()       |param()       |
                  |  +-------------------------------+       +----->        +------>       |
                  |                                 +--------+     +----+---+      +-------+
                  |                                                     |
                  |       notify_param()                                |
                  +-----------------------------------------------------+


*/

#ifdef BLE_APP_CONFIG
#include  <blefw_conf.h>
#endif

#include <stdlib.h>
#include "ptp_core.h"
#include "app_ble.h"
#include "ble_firmware.h"
#include "common_tables.h"
#include "clock_interface.h"
#include "ptp_interrupt.h"
#include "stm32f4xx_nucleo_add_led.h"
#include "RTCP_core.h"
#include "media_interrupt.h"

/**********Control service UUID specification********/
static const uint8_t sync_control_service_uuid[16] = { 0x66, 0x9a, 0x0c,
            0x20, 0x00, 0x08, 0x96, 0x9e, 0xe3, 0x11, 0x9f, 0xb1, 0xf0, 0xf2, 0x73,
            0xd9 };

static const uint8_t  sync_control_TXchar_uuid[16] = { 0x66,0x9a,0x0c,
            0x20,0x00,0x08,0x96,0x9e,0xe3,0x11,0x9f,0xb1,0xe1,0xf2,0x73,
            0xd9};

static app_service_t ctrl_sync_service;                  /*service structure (used for BLE services definition)*/
static app_attr_t ctrl_sync_tx_att;                      /*attribute structure (used for BLE attribute definition)*/
static uint8_t num_peer_device;                          /*store the number of peer devices connected*/
static ctrl_mode ctrol_op_mode = CTRL_STATIC_MODE;       /*defines the operation mode of the ctrl sync protocol*/                 

static ctrl_status_table CTRL_SYNC_STR[EXPECTED_NODES]; /*synchonization control table*/
volatile uint8_t Cinterval_CTRL_Started = 0;

static uint8_t static_parm_updated = FALSE;


/**********************Static Func Def*************************/
static void Ctrl_Sync_error_handler(void);
static void Ctrl_input_packet_process(uint16_t chandler, uint16_t attrhandler, uint8_t data_length, uint8_t *att_data, tClockTime arval_time);
static uint8_t parse_ctrl_sync_packet_header(uint8_t * data, uint8_t data_len, ctrl_sync_hdr * hdr);
static uint8_t  parse_ctrl_init_packet(uint8_t * data, uint8_t data_len,ctrl_init_packet * init_pack);
static ctrl_status_entry * ctrl_get_status_entry(uint16_t chandler);
static void process_init_packet(ctrl_init_packet * init_pack, uint16_t chandler);
static ctrl_status_entry * ctrl_set_new_status_entry(uint16_t chandler);
static uint8_t validate_connection_id(uint16_t chandler);
static ctrl_status_entry * ctrl_get_new_entry(void);
static uint16_t ctrl_get_source_id (uint8_t index);
static void send_ctrl_sync_packet(uint8_t entry_idx, uint8_t pkt_type);
static uint8_t create_ctrl_init_packet( uint8_t connect_id,ctrl_init_packet * init_pkt_str, uint8_t *buff );
static uint8_t create_ctrl_packet_hdr( uint8_t ctrl_entry_idx ,uint8_t cpkt_type, ctrl_sync_hdr * hdr, uint8_t *buff);
 static uint8_t create_ctrl_report_packet (uint8_t idx,ctrl_init_packet * init_pkt_str, uint8_t *buff );
 static uint8_t  parse_ctrl_src_report_packet(uint8_t * data, uint8_t data_len, ctrl_src_report_packet * src_report_pack);
 static void process_src_report_packet(ctrl_src_report_packet * src_report_pack, uint16_t chandler);
/*************************************************************/

/**
  * @brief  CTRL_get_control_table This function return a pointer to the control table.
  * @retval : ctrl_status_table *. pointer to the control table.
  *
  */

ctrl_status_table * CTRL_get_control_table(void)
{
    return CTRL_SYNC_STR;
}




/**
  * @brief  Ctrl_set_op_mode This function initializes the control synchronization protocol.
  * @param  app_profile_t * profile : profile to associate the service.
  * @retval : none.
  */

void Ctrl_set_op_mode(ctrl_mode op_mode){
    ctrol_op_mode = op_mode;
}


/**
  * @brief  Ctrl_Sync_init: 
  * This function initializes the control synchronization protocol.
  * @param  app_profile_t * profile : profile to associate the service.
  * @retval : none.
  */

void Ctrl_Sync_init(app_profile_t * profile)
{
      /* configure the  CTRL synchronization service and its characteristics */
       APP_Status ret;
       /*1. configure the Ctrl_Sync service and associate it to the app_profile*/
      COPY_VAR(ctrl_sync_service.ServiceUUID,sync_control_service_uuid);
      ctrl_sync_service.service_uuid_type=UUID_TYPE_128;
      ctrl_sync_service.service_type=SECONDARY_SERVICE;
      ctrl_sync_service.max_attr_records=3;
      /*copy the and associate the service to the BLE application profile*/
      ret = APP_add_BLE_Service(profile,&ctrl_sync_service);
      if(ret!=APP_SUCCESS) Ctrl_Sync_error_handler();
      /*2. configure the  Ctrl_Sync attribute and associate it to the Ctrl_Sync service*/
      COPY_VAR(ctrl_sync_tx_att.CharUUID,sync_control_TXchar_uuid);
      ctrl_sync_tx_att.charUuidType = UUID_TYPE_128;
      ctrl_sync_tx_att.charValueLen = 12;
      ctrl_sync_tx_att.charProperties = CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RESP;
      ctrl_sync_tx_att.secPermissions = ATTR_PERMISSION_NONE;
      ctrl_sync_tx_att.gattEvtMask = GATT_NOTIFY_ATTRIBUTE_WRITE;
      ctrl_sync_tx_att.encryKeySize=16;
      ctrl_sync_tx_att.isVariable=1;
      ret= APP_add_BLE_attr(&ctrl_sync_service,&ctrl_sync_tx_att);
    if(ret!=APP_SUCCESS)Ctrl_Sync_error_handler();
}



/**
  * @brief  This function process the input packets arriving form the BLE interface.
  * @param uint16_t chandler : connection handler associated ;
  *   @param ctrl_report_src_packet * src_report: src report data-structure
  *   @param uint8_t * buff : packet buffer
  * @retval : none.
  */
void Ctrl_input_packet_process(uint16_t chandler,
                            uint16_t attrhandler, 
                            uint8_t data_length, 
                            uint8_t *att_data, 
                            tClockTime arval_time)
{

      uint8_t ret;
      ctrl_sync_hdr ctrl_hdr;
      


/*enable notify*/
      if(attrhandler == ctrl_sync_tx_att.CharHandle + 2)
        {
          ctrl_status_entry *st = ctrl_get_status_entry(chandler);
            
               if(att_data[0] == 0x01){
                st->notify_enable=TRUE;
                }else{
                st->notify_enable=FALSE;
                }
        
/*process packet*/
        }else if(attrhandler == ctrl_sync_tx_att.CharHandle+1){
           /*parse the header*/
           ret = parse_ctrl_sync_packet_header(att_data,data_length,&ctrl_hdr);
           if(ret == 0) Ctrl_Sync_error_handler(); /*something is worng*/

           if(ctrl_hdr.pkt_type == INITIATOR){
            /*process init_packet*/
                  ctrl_init_packet init_pack;
                  init_pack.header = ctrl_hdr;
                  ret += parse_ctrl_init_packet(att_data+ret, data_length-ret, &init_pack);
                  if (ret!=CTRL_INIT_PCK_SIZE)Ctrl_Sync_error_handler(); /*something is worng*/
                  process_init_packet(&init_pack,chandler);
                
           }else if (ctrl_hdr.pkt_type == REPORT_SRC){
             /*process report_source_ctrl_packet*/
              ctrl_src_report_packet src_report_pkt;
              src_report_pkt.header = ctrl_hdr;
              ret += parse_ctrl_src_report_packet (att_data+ret,data_length-ret,&src_report_pkt);
              if (ret!=CTRL_RPC_PCK_SIZE)Ctrl_Sync_error_handler(); /*something is worng*/
              process_src_report_packet(&src_report_pkt, chandler);
              
              

           }else if (ctrl_hdr.pkt_type == REPORT_RRC){
                  /*process report_receiver_ctrl_packet*/
                  /*FIXME: not yet implemented*/

           }
           
           
           
           
#ifdef DEBUG_MDA
           /*used to show how the synchronization works:
                 when the value slave_max_delay_diff == 0 then the device call
                 a function that could be used for example to stream audio synchronously.
           
                in this case we just switch on and off a led. 
           */
      if(ctrl_hdr.pkt_type == REPORT_RRC ||  ctrl_hdr.pkt_type == REPORT_SRC || ctrl_hdr.pkt_type == INITIATOR)
      {
           if(CTRL_SYNC_STR[0].sync_param.slave_max_delay_diff==0)
                  {
                    BSP_ADD_LED_On(ADD_LED2);        
                    BSP_ADD_LED_Off(ADD_LED2);
                  }else{
                  uint32_t period =  (CTRL_SYNC_STR[0].sync_param.slave_max_delay_diff*100);
                  MDA_update_interrupt(period,0);
                  }
       } 
#else
      
       if(ctrl_hdr.pkt_type == REPORT_RRC ||  ctrl_hdr.pkt_type == REPORT_SRC || ctrl_hdr.pkt_type == INITIATOR)
       {
          
       }
       
       
#endif   

        }


      //st = ctrl_get_status_table(chandler);
}


/**
  * @brief  This function return the status of the synchronization protocol.
  * @param  uint16_t chandler connection handler that identified the peer device
  * @retval : control synchronization status.
  */
ctrl_sync_status  Ctrl_Get_sync_status(uint16_t chandler){

      ctrl_sync_status stus_ret;
      ctrl_status_entry * tmp_entry;
      tmp_entry = ctrl_get_status_entry(chandler);
      if (tmp_entry == NULL)Ctrl_Sync_error_handler();/*something was wrong*/
      stus_ret = tmp_entry->local_sync_status;
      return (stus_ret);

}


/**
  * @brief  This function return and specific remote device synchonization parmeters.
  * @param  uint16_t chandler connection handler that identified the peer device
  * @retval : control synchronization status.
  */
ctrl_sync_param * Ctrl_Get_remote_sync_param(uint16_t chandler)
{
      ctrl_sync_param * temp_sync_param;
      ctrl_status_entry * temp_entry;

      temp_entry=ctrl_get_status_entry(chandler);

      if (temp_entry != NULL)
             temp_sync_param = &(temp_entry->sync_param);

return (temp_sync_param);
}


/**
  * @brief  This function is used at the beginning of the media transmission
                  to sent the initialization parameters to all the synchronization-media-clients.
  * @param  uint8_t no_receivers : indicates the number of receivers.
  * @param  uint8_t no_packets : optional number of packets to transmit.
  * @retval : none
  */
void Ctrl_Sync_start(uint8_t no_peers, uint8_t no_packets)
{
      /*initialy is  considered only an static configuration based in connection interval configuration*/
uint8_t i;
num_peer_device = no_peers;

if(NET_get_device_role() == DEVICE_CENTRAL)
 {
      for(i=0; i < num_peer_device; i++)
      {
            CTRL_SYNC_STR[i].local_sync_status = UNSTARTED;
            CTRL_SYNC_STR[i].connect_id = ctrl_get_source_id(i);
            CTRL_SYNC_STR[i].seq_id = i+1;
            CTRL_SYNC_STR[i].total_peers = no_peers;
            CTRL_SYNC_STR[i].sync_param.packet_count = 0;
            /*(optionally)the following values could be acquiered using the ptp protocol or another*/
            /*the following values has been setting up statically based on the connection interval configuration*/
            if(static_parm_updated==FALSE){
            CTRL_SYNC_STR[i].sync_param.tx_delay = TX_INTERVAL;
            CTRL_SYNC_STR[i].sync_param.slave_max_delay_diff =EXPECTED_DELAY * (1-i);
            }
            
            CTRL_enable_notify(CTRL_SYNC_STR[i].connect_id); /*not_needed*/
            CTRL_SYNC_STR[i].notify_enable=TRUE; /*fixme: remove this variable for the controller is not really needed*/
            CTRL_SYNC_STR[i].pending_pack_type = INITIATOR;
            CTRL_SYNC_STR[i].pending_pack = FALSE;
      }
 }else if(NET_get_device_role() == DEVICE_PERISPHERAL){
     

     for (i=0; i<num_peer_device; i++ ){
          CTRL_SYNC_STR[i].local_sync_status = UNSTARTED;
          CTRL_SYNC_STR[i].connect_id = ctrl_get_source_id(i);
          CTRL_SYNC_STR[i].seq_id = 0;
          CTRL_SYNC_STR[i].total_peers = no_peers;
          CTRL_SYNC_STR[i].sync_param.packet_count = 0;
      } 
 }


/*INITIALIZE THE CONNECTION INTERVAL INTERRUPTION USED TO SEND DATA SYNCHRONOUSLY*/
#if defined (TEST_CTRL_SERVER)          
     BlueNRG_ConnInterval_Init(10); /*this must be done by the top API*/
#endif     

}


/**
  * @brief  This function is to transmit synchronously ctrl packets using the connection interval
  			interrupt.
  * @param  : ctrl_status_entry * CTRL_SYNC_STR:  pointer to the entry for which will sent a message. 
  * @param  : uint8_t index :  index of the entre for which will sent a message. 
  * @retval : none
  */
void Ctrl_Sync_send_pending_packets( ctrl_status_entry * CTRL_SYNC_STR, uint8_t index){
     
      
      if (CTRL_SYNC_STR->pending_pack == TRUE &&
                CTRL_SYNC_STR->notify_enable == TRUE 
                  ){
                  switch (CTRL_SYNC_STR->pending_pack_type)
                  {
                     case INITIATOR:
                     {
                        send_ctrl_sync_packet(index,INITIATOR);
                        CTRL_SYNC_STR->pending_pack_type=REPORT_SRC;
                        CTRL_SYNC_STR->pending_pack = FALSE;
                     }
                     break;
                     case REPORT_SRC:
                     {
                        send_ctrl_sync_packet(index,REPORT_SRC);
                        CTRL_SYNC_STR->pending_pack_type=REPORT_SRC;
                        CTRL_SYNC_STR->pending_pack = FALSE;
                     }
                     break;
                     case REPORT_RRC:
                     {

                     }
                     break;
                  }
            }
}


/**
  * @brief  This function is used as the server main control process
                  interrupt.
  * @param  : none
  * @retval : none
  */

void Ctrl_Sync_server_main(void)
{
      /*this process simply wait for an input packet to process or if a command arrive from the application*/
  //uint8_t i;
  event_t * event;
  event = (event_t *)HCI_Get_Event_CB();

            if(event!=NULL){
                  /*get the events associated to CTL_protocol*/

                  switch (event->event_type){
                        
                  case EVT_BLUE_GATT_ATTRIBUTE_MODIFIED:
                    {
                      uint8_t hw_version = get_harware_version();
                      if(hw_version==IDB05A1){
                        evt_gatt_attr_modified_IDB05A1 *evt = (evt_gatt_attr_modified_IDB05A1*)event->evt_data;
                         Ctrl_input_packet_process(
                                                    evt->conn_handle,
                                                    evt->attr_handle,
                                                    evt->data_length,
                                                    evt->att_data,
                                                    event->ISR_timestamp
                                                   );
                      }
                    }
                    break;
                  
                   case EVT_BLUE_GATT_NOTIFICATION :
                    {
                         evt_gatt_attr_notification *evt = (evt_gatt_attr_notification*)event->evt_data;
                         Ctrl_input_packet_process(evt->conn_handle,
                                                   evt->attr_handle,
                                                   evt->event_data_length,
                                                   evt->attr_value,
                                                   event->ISR_timestamp);

                        }
                        break;

                  }

            }

}

/**
  * @brief  This function is used as the client main control process
                  interrupt.
  * @param  : none
  * @retval : none
  */
void Ctrl_Sync_client_main(void){
      /*this process simply wait for an input packet to process or if a command arrive from the application*/
  //uint8_t i;
  event_t * event;
  event = (event_t *)HCI_Get_Event_CB();

            if(event!=NULL){
                  /*get the events associated to CTL_protocol*/

                  switch (event->event_type){
                        
                  case EVT_BLUE_GATT_ATTRIBUTE_MODIFIED:
                    {
                      uint8_t hw_version = get_harware_version();
                      if(hw_version==IDB05A1){
                        evt_gatt_attr_modified_IDB05A1 *evt = (evt_gatt_attr_modified_IDB05A1*)event->evt_data;
                         Ctrl_input_packet_process(
                                                    evt->conn_handle,
                                                    evt->attr_handle,
                                                    evt->data_length,
                                                    evt->att_data,
                                                    event->ISR_timestamp
                                                   );
                      }
                    }
                    break;
                  
                   case EVT_BLUE_GATT_NOTIFICATION :
                    {
                         evt_gatt_attr_notification *evt = (evt_gatt_attr_notification*)event->evt_data;
                         Ctrl_input_packet_process(evt->conn_handle,
                                                   evt->attr_handle,
                                                   evt->event_data_length,
                                                   evt->attr_value,
                                                   event->ISR_timestamp);

                     }
                     break;

                  }

            }

}



/**
  * @brief  This function parse and input control-sync init or report packet header.
  * @param  uint8_t * data : buffer input data.
  *   @param  uint8_t data_len : total length of the input data.
  *   @param  ctrl_sync_hdr * hdr : header data_structure.
  * @retval : payload pointer offset.
  */
/*
static uint8_t  parse_ctrl_sync_packet_header_reciver(uint8_t * data, uint8_t data_len, ctrl_sync_hdr * hdr)
{
      uint8_t * p;
            if(data_len < CTRL_HDR_PCK_SIZE)return 0;

      p=data;

      hdr->pkt_type = *p++;
      hdr->connect_id = ((*p << 8) | *(p+1));
      p+=1;
      hdr->total_peers = *p++;
      hdr->seq_id = *p++;

      return (p-data);
}
*/

/**
  * @brief  This function parse and input control-sync ipayload.
  * @param  uint8_t * data : buffer input data.
  *   @param  uint8_t data_len : total length of the input data.
  *   @param  ctrl_init_packet * ctrl_pack : init_data_structure.
  * @retval : payload pointer offset.
  */

static uint8_t  parse_ctrl_init_packet(uint8_t * data, uint8_t data_len, ctrl_init_packet * init_pack)
{
      uint8_t * p = (uint8_t *)data;
      uint16_t temp_val;
      
      temp_val = (*p++) << 8; 
      init_pack->init_sync_parter.packet_count = temp_val | *p++;
      temp_val = (*p++) << 8;
      init_pack->init_sync_parter.tx_delay = temp_val | *p++;
      temp_val = (*p++) << 8;
      init_pack->init_sync_parter.slave_max_delay_diff= temp_val | *p;
      return(p-data);

}


/**
  * @brief  This function parse and input report-sync ipayload.
  * @param  uint8_t * data : buffer input data.
  *   @param  uint8_t data_len : total length of the input data.
  *   @param  ctrl_src_report_packet * ctrl_pack : report_data_structure.
  * @retval : payload pointer offset.
  */
static uint8_t  parse_ctrl_src_report_packet(uint8_t * data, uint8_t data_len, ctrl_src_report_packet * report_pack)
{
     uint8_t * p = (uint8_t *)data;
      uint16_t temp_val;
      
      temp_val = (*p++) << 8; 
      report_pack->init_sync_parter.packet_count = temp_val | *p++;
      temp_val = (*p++) << 8;
      report_pack->init_sync_parter.tx_delay = temp_val | *p++;
      temp_val = (*p++) << 8;
      report_pack->init_sync_parter.slave_max_delay_diff= temp_val | *p;
      return(p-data);
}


/**
  * @brief  This function allows to sink devices enable the notify.
  * @param uint8_t chandler: peer connection handler;
  * @retval : (1) if the process is successed otherwise (0).
  */

uint8_t CTRL_enable_notify(uint16_t chandler){

      uint8_t notify_enable_data [] = {0x01,0x00};

      struct timer t;
       Timer_Set(&t, CLOCK_SECOND*10);

        while(aci_gatt_write_charac_descriptor(chandler,ctrl_sync_tx_att.Associate_CharHandler+2,2,notify_enable_data)==BLE_STATUS_NOT_ALLOWED)
        {
          if(Timer_Expired(&t))return 0;/*error*/
        }
      return 1;

}

/**/


/**
  * @brief  This function send a control sync init or report packet.
  * @param uint8_t entry_idx: destination ID;
  *   @param  uint8_t pkt_type : packet type
  * @retval : void.
  */

static void send_ctrl_sync_packet(uint8_t entry_idx, uint8_t pkt_type){

      tBleStatus res_ble;
      uint8_t ret;
      uint8_t tx_buffer[CTRL_INIT_PCK_SIZE+1];
      ctrl_init_packet init_pck;
      uint16_t temp_chandler = ctrl_get_source_id(entry_idx); 
      
      //ctrl_report_src_packet src_report_pck;
      

      if(pkt_type == INITIATOR){
            ret = create_ctrl_init_packet(entry_idx,&init_pck,tx_buffer);

      }else if(pkt_type == REPORT_SRC){
            //
            ret = create_ctrl_report_packet(entry_idx,&init_pck,tx_buffer);
      }

      if(ret==0)Ctrl_Sync_error_handler();
      res_ble = aci_gatt_write_without_response(temp_chandler,ctrl_sync_tx_att.Associate_CharHandler + 1,CTRL_INIT_PCK_SIZE+1,tx_buffer);
      if(res_ble!= BLE_STATUS_SUCCESS)Ctrl_Sync_error_handler();

}

/**
  * @brief  This function creates a control sync init packet.
  * @param uint8_t ctrl_entry_idx;
  *   @param uint8_t cpackets: num_packets to transmit default use 0.
  *   @param ctrl_init_packet * init_pkt_str : control init datastructure
  *   @param uint8_t * buff : packet buffer
  * @retval : payload pointer.
  */

  static uint8_t create_ctrl_init_packet( uint8_t idx,ctrl_init_packet * init_pkt_str, uint8_t *buff )
  {
      uint8_t ret;
      uint8_t * p = buff;

      ret = create_ctrl_packet_hdr (idx,INITIATOR,&(init_pkt_str->header),buff);

      init_pkt_str->init_sync_parter.packet_count=CTRL_SYNC_STR[idx].sync_param.packet_count;
      init_pkt_str->init_sync_parter.tx_delay=CTRL_SYNC_STR[idx].sync_param.tx_delay;
      init_pkt_str->init_sync_parter.slave_max_delay_diff=CTRL_SYNC_STR[idx].sync_param.slave_max_delay_diff;
      p += ret;

      *p++ =  ((init_pkt_str->init_sync_parter.packet_count & 0x00FF)>>8);
      *p++ =  (init_pkt_str->init_sync_parter.packet_count & 0xFF);

      *p++ =  ((init_pkt_str->init_sync_parter.tx_delay & 0x00FF)>>8);
      *p++ =  (init_pkt_str->init_sync_parter.tx_delay & 0xFF);

      *p++ =  ((init_pkt_str->init_sync_parter.slave_max_delay_diff & 0x00FF)>>8);
      *p =  (init_pkt_str->init_sync_parter.slave_max_delay_diff & 0xFF);

      return (p - buff);



  }
  
/**
  * @brief  This function creates a report ctrl packet.
  * @param uint8_t ctrl_entry_idx;
  *   @param uint8_t cpackets: num_packets to transmit default use 0.
  *   @param ctrl_init_packet * init_pkt_str : control init datastructure
  *   @param uint8_t * buff : packet buffer
  * @retval : payload pointer.
  */  
  static uint8_t create_ctrl_report_packet (uint8_t idx,ctrl_init_packet * init_pkt_str, uint8_t *buff )
  {     
    uint8_t ret;
      uint8_t * p = buff;

      ret = create_ctrl_packet_hdr (idx,REPORT_SRC,&(init_pkt_str->header),buff);

      init_pkt_str->init_sync_parter.packet_count=CTRL_SYNC_STR[idx].sync_param.packet_count;
      init_pkt_str->init_sync_parter.tx_delay=CTRL_SYNC_STR[idx].sync_param.tx_delay;
      init_pkt_str->init_sync_parter.slave_max_delay_diff=CTRL_SYNC_STR[idx].sync_param.slave_max_delay_diff;
      p += ret;

      *p++ =  ((init_pkt_str->init_sync_parter.packet_count & 0x00FF)>>8);
      *p++ =  (init_pkt_str->init_sync_parter.packet_count & 0xFF);

      *p++ =  ((init_pkt_str->init_sync_parter.tx_delay & 0x00FF)>>8);
      *p++ =  (init_pkt_str->init_sync_parter.tx_delay & 0xFF);

      *p++ =  ((init_pkt_str->init_sync_parter.slave_max_delay_diff & 0x00FF)>>8);
      *p =  (init_pkt_str->init_sync_parter.slave_max_delay_diff & 0xFF);

      return (p - buff);
  }


  /**
  * @brief  This function creates a control sync header.
  * @param uint8_t ctrl_entry_idx;
  *   @param uint8_t cpkt_type: packet type.
  *   @param ctrl_sync_hdr * hdr : ptp_header data-structure
  *   @param uint8_t * buff : packet buffer
  * @retval : payload pointer.
  */

static uint8_t create_ctrl_packet_hdr( uint8_t ctrl_entry_idx ,uint8_t cpkt_type, ctrl_sync_hdr * hdr, uint8_t *buff)
{
      uint8_t * p = buff;

      hdr->pkt_type = cpkt_type;
      hdr->connect_id = CTRL_SYNC_STR[ctrl_entry_idx].connect_id;
      hdr->total_peers = CTRL_SYNC_STR[ctrl_entry_idx].total_peers;
      hdr->seq_id = CTRL_SYNC_STR[ctrl_entry_idx].seq_id;

      *p++=(hdr->pkt_type);
      *p++=((hdr->connect_id & 0xFF00) >> 8);
      *p++=((hdr->connect_id & 0xFF));
      *p++=hdr->total_peers;
      *p++=hdr->seq_id;

      return (p-buff);
}

/**
  * @brief  This function call the network.c primitives to retrieve a conneciton handler
  * @param  uint8_t index : indicates the low level entry for the network.c primitives.
  * @retval : connection handler.
  */
static uint16_t ctrl_get_source_id (uint8_t i)
{
      uint16_t tmp_chandler;

      tmp_chandler = NET_get_chandler_by_index (i);

      return tmp_chandler;

}


/**
  * @brief  This function returns the control table.
  * @param  uint16_t chandler : the connection handler defines the connection_id between pair devices.
  * @retval :  control table pointer.
  */

static ctrl_status_entry *ctrl_get_status_entry(uint16_t chandler){

ctrl_status_entry * ret = CTRL_SYNC_STR;

      while(ret != NULL)
      {
            if (ret->connect_id == chandler) break;
            ret++;

      }

return ret;
}


 
/**
  * @brief  This function validate the connection id.
  * @param  uint16_t chandler : serching parameter.
  * @retval :  (TRUE) if the connection id exist otherwise false.
  */

static uint8_t validate_connection_id(uint16_t chandler){

    uint8_t ret = FALSE;
    ctrl_status_entry * ptr;

    for (ptr = CTRL_SYNC_STR; ptr !=NULL; ptr++){
       if (ptr->connect_id == chandler){
            ret = TRUE;
            break;
       }
    }
    return  ret;
}



/**
  * @brief  This function return a new entry slot it exist.
  * @param  void.
  * @retval :  ctrl_status_entry * : pointer to a free ctrl table entry.
  */
static ctrl_status_entry * ctrl_get_new_entry(void)
{
      ctrl_status_entry * ptr;
            for (ptr = CTRL_SYNC_STR; ptr !=NULL; ++ptr){
                  if (ptr->local_sync_status== UNSTARTED)
                        break;

            }

return ptr;
}



/**
  * @brief  This function returns a new control table entry or null.
  * @param  uint16_t chandler : serching parameter.
  * @retval :  ctrl_status_entry * : pointer to a control status entry.
  */
static ctrl_status_entry * ctrl_set_new_status_entry(uint16_t chandler)
{
      ctrl_status_entry * ptr;
      //uint8_t i;
      //uint16_t thandler;

      if (NET_valiadate_chandler(chandler) == FALSE  || num_peer_device == 0 || validate_connection_id(chandler)==FALSE )
            return NULL;

      ptr = ctrl_get_new_entry();

return ptr;
}


/**
  * @brief  This function parse and input control-sync init or report packet header.
  * @param  uint8_t * data : buffer input data.
  *   @param  uint8_t data_len : total length of the input data.
  *   @param  ctrl_sync_hdr * hdr : header data_structure.
  * @retval : payload pointer offset.
  */

static uint8_t parse_ctrl_sync_packet_header(uint8_t * data, uint8_t data_len, ctrl_sync_hdr * hdr)
{
      uint8_t * p;

      if(data_len < CTRL_HDR_PCK_SIZE)
      {
            /*something is wrong return 0*/
            return 0;
      }

      p=data;

      hdr->pkt_type =         *p++;
      uint16_t temp_cid =      ((*p++<<8));
      hdr->connect_id = temp_cid | (*p++);
      hdr->total_peers =      *p++;
      hdr->seq_id =           *p++;

      return (p-data);

}


/**
  * @brief  This function parse and input control-sync init packet header.
  * @param  uint8_t * data : buffer input data.
  *   @param  uint8_t data_len : total length of the input data.
  *   @param  ctrl_init_packet * ctrl_pack : init_data_structure.
  * @retval : payload pointer offset.
  */

/*static uint8_t  parse_ctrl_init_packet(uint8_t * data, uint8_t data_len, ctrl_init_packet * init_pack)
{
      uint16_t * p = (uint16_t *)data;
      init_pack->init_sync_parter.packet_count             = p[0];
      init_pack->init_sync_parter.tx_delay                 = p[1];
      init_pack->init_sync_parter.slave_max_delay_diff     = p[2];

      return CTRL_SYNC_PARM_SIZE;

}*/


/**
  * @brief  This function process a control sync init packet.
  * @param ctrl_init_packet * init_pack : input init packet to be processed;
  * @param uint16_t chandler : input init packet to be processed;
  * @retval : none.
  */

static void process_init_packet(ctrl_init_packet * init_pack, uint16_t chandler)
{
      ctrl_status_table *st;

      st = ctrl_set_new_status_entry(chandler);
      if (st==NULL) return; /*wrong chandler or devices has not been correctly associated*/

      st->local_sync_status = IDLE;
      st->connect_id = init_pack->header.connect_id;
      st->seq_id = init_pack->header.seq_id;
      st->total_peers = init_pack->header.total_peers;
      st->sync_param.packet_count = 0;
      st->sync_param.tx_delay = init_pack->init_sync_parter.tx_delay;
      st->sync_param.slave_max_delay_diff = init_pack->init_sync_parter.slave_max_delay_diff;
}


/**
  * @brief  This function process a source report packet.
  * @param ctrl_init_packet * init_pack : input init packet to be processed;
  * @param uint16_t chandler : input init packet to be processed;
  * @retval : none.
  */


static void process_src_report_packet(ctrl_src_report_packet * src_report_pack, uint16_t chandler)
{
  ctrl_status_table *st;

      st = ctrl_set_new_status_entry(chandler);
      if (st==NULL) return; /*wrong chandler or devices has not been correctly associated*/

      st->local_sync_status = IDLE;
      st->connect_id = src_report_pack->header.connect_id;
      st->seq_id = src_report_pack->header.seq_id;
      st->total_peers = src_report_pack->header.total_peers;
      st->sync_param.packet_count = 0;
      st->sync_param.tx_delay = src_report_pack->init_sync_parter.tx_delay;
      st->sync_param.slave_max_delay_diff = src_report_pack->init_sync_parter.slave_max_delay_diff;
      
}


/**
  * @brief  This function stops the program execution in case of error.
  * param: none
  * @retval : none.
  */

static void Ctrl_Sync_error_handler(void)
{

      while(1)
      {

      }
}

/**
  * @brief  
  * this fuction guarantee the order of the ctrl tx.
  * param: none 
  * @retval : none.
  */
static  uint8_t positive_tx(uint8_t tx_index){
  /*this fuction avoids retransmission*/
  const ctrl_status_entry * ctrl_ptr = &CTRL_SYNC_STR[0];
  
  if (tx_index != 0 && ctrl_ptr->pending_pack==TRUE){
    return FALSE;
  }
  
  return TRUE;
  
  
}


/**
  * @brief  This function deals to the connection interval Iterruption.
  * param: none 
  * @retval : none.
  */
volatile uint8_t ctrl_sync_id =0;
void Ctrl_Sync_cinterval_IRQ_handler(uint8_t connection_id){

  uint8_t index= connection_id;
  if(!positive_tx(index))return;
  Ctrl_Sync_send_pending_packets(&CTRL_SYNC_STR[index],index);
 
}

/**
  * @brief CTRL_sync: 
  *This function automatically send the control synchonization parameters
  * to all the peer devices.
  * @parm: none
  * @retval : none.
  */

void CTRL_sync()
{
   uint8_t i = num_peer_device;
    ctrl_status_entry * ptr = CTRL_SYNC_STR;
    for (;i > 0; i-- ){
      switch (ptr->local_sync_status){
      case UNSTARTED:
        {
          /*send initial_packet*/
          ptr->pending_pack_type = INITIATOR;
          ptr->pending_pack=TRUE;
        }
        break;
      case IDLE:
        {
          /*send report_packet*/
          ptr->pending_pack_type = REPORT_SRC;
          ptr->pending_pack=TRUE;
        }
        break;
      }
    ptr++;  
    }
}

/**
  * @brief CTRL_sync_send_sp_pk: 
  *This function send and specific packet type to all the peer devices
  * @parm: none
  * @retval : none.
  */
void CTRL_sync_send_sp_pk(uint8_t PK_type)
{
    uint8_t i = num_peer_device;
    ctrl_status_entry * ptr = CTRL_SYNC_STR;
    
    for (; i>0; i--){
      ptr->pending_pack_type = PK_type;
      ptr->pending_pack=TRUE;
      ptr++;
    }
}


/**
  * @brief UPDATE_SYNC_IRQ: 
  *This function us used to update the SYNC_parametes as soon the PTP process is completed.
  * @parm: none
  * @retval : none.
  */
void UPDATE_SYNC_IRQ(){
  uint8_t i;
  uint16_t tmp_delay_diff;
  ptp_status_table * tmp_ptp_tbl;
  static ctrl_status_entry * tmp_ctrl_entry;
  ctrl_sync_param * tmp_prm;
  
  tmp_ptp_tbl = PTP_GET_status_tbl();
  
  tmp_delay_diff = abs((tmp_ptp_tbl[num_peer_device-1].timers.t0 - tmp_ptp_tbl[0].timers.t0));
  
  
  for (i=0;i <num_peer_device; i++, ++tmp_ptp_tbl ){
    tmp_ctrl_entry = ctrl_get_status_entry (tmp_ptp_tbl->Chandler);
    tmp_prm = &(*tmp_ctrl_entry).sync_param;
    (*tmp_prm).tx_delay = (uint32_t)abs((tmp_ptp_tbl->timers.t3 - tmp_ptp_tbl->timers.t1));
    (*tmp_prm).slave_max_delay_diff = tmp_delay_diff * (tmp_ctrl_entry->total_peers - tmp_ctrl_entry->seq_id);
    tmp_ctrl_entry->pending_pack=TRUE;
    /*
    tmp_ctrl_entry->sync_param.tx_delay = (uint32_t)abs((tmp_ptp_tbl->timers.t3 - tmp_ptp_tbl->timers.t1));
    tmp_ctrl_entry->sync_param.slave_max_delay_diff = tmp_delay_diff * (tmp_ctrl_entry->total_peers - tmp_ctrl_entry->seq_id);
    tmp_ctrl_entry->pending_pack=TRUE;
    */
  }
}



/**
  * @brief CTRL_update_static_parameters 
  * This function is used to update the synchonization parameters on static mode 
  * you may used when you set up custom connetion paramiters (i.e when there isn't used 
  * the default parameters reported on <blefw_conf.h> to stabhish the multinode connection)

                                    Max delay
                                     <----->
                                      C2-C3
                                max delay
                              <------------>
                                 C1-C3

                              C1     C2    C3        C1    C2    C3
                               +     +     +         +     +     +
                               |     |     |         |     |     |
                               |     |     |         |     |     |
                               +-----+-----+---------+-----+-----+-->

                               ^ connection iter^al  ^
                            C1 +---------------------+-------------->

                                     ^ connection inter^al ^
                            C2 +-----+---------------------+-------->





  * @parm: none
  * @retval : none.
  */
void CTRL_update_static_parameters(){
  
  /* To compute the inter destination delay between multiple receives it is necessa
   * 
   *
   *
  */  
  uint8_t i;
  float inter_delay[num_peer_device];
  float conn_inval[num_peer_device];
  if(num_peer_device==0)return;/*there not connections already stablished*/
  connection_t * tmp_conn[num_peer_device];
  ptp_status_table * tmp_ptp_tbl;
  ctrl_status_entry * tmp_ctrl_entry;
  ctrl_sync_param * tmp_prm;
  
  NET_get_all_connections(tmp_conn);          /* get pointer to all connections associated to the network*/
  
  if(num_peer_device == 2 ){
  /*optimization for two nodes*/
  
  inter_delay[0] = ((tmp_conn[0]->cconfig->clengthmin + tmp_conn[0]->cconfig->clengthmax)/2)*(0.625f);
  inter_delay[1] = ((tmp_conn[1]->cconfig->clengthmin + tmp_conn[1]->cconfig->clengthmax)/2)*(0.625f);
 
  conn_inval[0] = ((tmp_conn[0]->cconfig->cintervalmin + tmp_conn[0]->cconfig->cintervalmax)/2)* (1.25f);
  conn_inval[1] = ((tmp_conn[1]->cconfig->cintervalmin + tmp_conn[1]->cconfig->cintervalmax)/2)* (1.25f);
  
  
  for (i=0;i <num_peer_device; i++ ){
    tmp_ctrl_entry = ctrl_get_status_entry (tmp_conn[i]->Connection_Handle);
      tmp_prm = &(*tmp_ctrl_entry).sync_param;
     (*tmp_prm).tx_delay = (uint32_t)conn_inval[i]; 
     (*tmp_prm).slave_max_delay_diff = (uint32_t)inter_delay[i];  
    }
  
  }else{
  
  
    /*generic implementation*/
  
  
  /*associate the conneciton parameters to all */
  for(i=0; i< num_peer_device; i++ ){
    
    inter_delay[i]=  ((tmp_conn[i]->cconfig->clengthmin + tmp_conn[i]->cconfig->clengthmax)/2)*(0.625f);
    conn_inval[i] = ((tmp_conn[i]->cconfig->cintervalmin + tmp_conn[i]->cconfig->cintervalmax)/2)* (1.25f);
    
  }
  
  /*fixme : THIS SECTION IS NOT COMPLETED*/
  
  /*computing the maximum delay per node*/
    
    for (i=0;i <num_peer_device; i++, ++tmp_ptp_tbl ){
        tmp_ctrl_entry = ctrl_get_status_entry (tmp_ptp_tbl->Chandler);
        tmp_prm = &(*tmp_ctrl_entry).sync_param;
        (*tmp_prm).tx_delay = (uint32_t)conn_inval[i]; 
        (*tmp_prm).slave_max_delay_diff = (uint32_t)inter_delay[i];
     
    }
  
  }  
 static_parm_updated = TRUE;
  
  
}
 





void CTRL_sync_IRQ_Handler(){
    if(!Cinterval_CTRL_Started){
      Cinterval_CTRL_Started=1;
#if defined(TEST_MEDIA_SYNC_SERVER)      
      BlueNRG_ConnInterval_IRQ_enable();
#endif      
  }
}




