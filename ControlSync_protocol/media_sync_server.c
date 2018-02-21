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


#include "ptp_core.h"
#include "app_ble.h"
#include "ble_firmware.h"
#include "common_tables.h"
#include "clock.h"
#include "ptp_interrupt.h"
#include "stm32f4xx_nucleo_add_led.h"
#include "media_sync_server.h"

/**********Control service UUID specification********/
static const uint8_t sync_control_service_uuid[16] = { 0x66, 0x9a, 0x0c,
            0x20, 0x00, 0x08, 0x96, 0x9e, 0xe2, 0x11, 0x9f, 0xb1, 0xf0, 0xf2, 0x73,
            0xd9 };

static const uint8_t  sync_control_TXchar_uuid[16] = { 0x66,0x9a,0x0c,
            0x20,0x00,0x08,0x96,0x9e,0xe2,0x11,0x9f,0xb1,0xe1,0xf2,0x73,
            0xd9};


static app_service_t ctrl_sync_service;                  /*service structure (used for BLE services definition)*/
static app_attr_t ctrl_sync_tx_att;                      /*attribute structure (used for BLE attribute definition)*/
static uint8_t num_peer_device;                          /*store the number of peer devices connected*/


typedef ctrl_status_entry ctrl_status_table;             /*used just to clarify*/

static ctrl_status_table CTRL_SYNC_STR[EXPECTED_NODES]; /*synchonization control table*/

volatile uint8_t Cinterval_CTRL_Started = 0;


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
static uint8_t  parse_ctrl_init_packet(uint8_t * data, uint8_t data_len, ctrl_init_packet * init_pack);
/*************************************************************/

/**
  * @brief  This function initializes the control synchronization protocol.
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
      ctrl_sync_service.service_type=PRIMARY_SERVICE;
      ctrl_sync_service.max_attr_records=7;
      /*copy the and associate the service to the BLE application profile*/
      ret = APP_add_BLE_Service(profile,&ctrl_sync_service);
      if(ret!=APP_SUCCESS) Ctrl_Sync_error_handler();
      /*2. configure the  Ctrl_Sync attribute and associate it to the Ctrl_Sync service*/
      COPY_VAR(ctrl_sync_tx_att.CharUUID,sync_control_TXchar_uuid);
      ctrl_sync_tx_att.charUuidType = UUID_TYPE_128;
      ctrl_sync_tx_att.charValueLen = 20;
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

           }else if (ctrl_hdr.pkt_type == REPORT_RRC){
                  /*process report_receiver_ctrl_packet*/

           }

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
            CTRL_SYNC_STR[i].seq_id = 0;
            CTRL_SYNC_STR[i].total_peers = no_peers;
            CTRL_SYNC_STR[i].sync_param.packet_count = 0;
            /*(optionally)the following values could be acquiered using the ptp protocol or another*/
            /*the following values has been setting up statically based on the connection interval configuration*/
            CTRL_SYNC_STR[i].sync_param.tx_delay = TX_INTERVAL;
            CTRL_SYNC_STR[i].sync_param.slave_max_delay_diff =EXPECTED_DELAY * (1-i);
            CTRL_SYNC_STR[i].notify_enable=TRUE; /*fixme: remove this variable for the controller is not really needed*/
            CTRL_SYNC_STR[i].pending_pack_type = INITIATOR;
            CTRL_SYNC_STR[i].pending_pack = TRUE;
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
     BlueNRG_ConnInterval_Init(10);

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
                        CTRL_SYNC_STR->pending_pack = FALSE;
                     }
                     break;
                     case REPORT_SRC:
                     {

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
      uint16_t * p = (uint16_t *)data;

      init_pack->init_sync_parter.packet_count = *p++;
      init_pack->init_sync_parter.tx_delay = *p++;
      init_pack->init_sync_parter.slave_max_delay_diff= *p++;

      return CTRL_INIT_PCK_PARAM;

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

/**
  * @brief  This function send a control sync init or report packet.
  * @param uint8_t entry_idx: destination ID;
  *   @param  uint8_t pkt_type : packet type
  * @retval : void.
  */

static void send_ctrl_sync_packet(uint8_t entry_idx, uint8_t pkt_type){

      tBleStatus res_ble;
      uint8_t ret;
      uint8_t tx_buffer[CTRL_INIT_PCK_SIZE];
      ctrl_init_packet init_pck;
      uint16_t temp_chandler = ctrl_get_source_id(entry_idx); 
      //ctrl_report_src_packet src_report_pck;

      if(pkt_type == INITIATOR){
            ret = create_ctrl_init_packet(entry_idx,&init_pck,tx_buffer);

      }else if(pkt_type == REPORT_SRC){
            //
      }

      if(ret==0)Ctrl_Sync_error_handler();
      res_ble = aci_gatt_write_without_response(temp_chandler,ctrl_sync_tx_att.Associate_CharHandler + 1,CTRL_INIT_PCK_SIZE,tx_buffer);
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
      *p++ =  (init_pkt_str->init_sync_parter.slave_max_delay_diff & 0xFF);

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

      hdr->pkt_type =         p[0];
      hdr->connect_id =       ((p[1]<<8) | (p[2]));
      hdr->total_peers =      p[3];
      hdr->seq_id =           p[4];

      return CTRL_HDR_PCK_SIZE;

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
  * @brief  This function stops the program execution in case of error.
  * @retval : none.
  */

static void Ctrl_Sync_error_handler(void)
{

      while(1)
      {

      }
}

/**
  * @brief  This function deals to the connection interval Iterruption.
  * @retval : none.
  */
volatile uint8_t ctrl_sync_id =0;
void Ctrl_Sync_cinterval_IRQ_handler(void){

  if(network_get_status())
  {
    switch (ctrl_sync_id){
      case 0:
        {
          ctrl_sync_id=1;
          Ctrl_Sync_send_pending_packets(&CTRL_SYNC_STR[0],0);
        }
        break;
        case 1:
        {
          ctrl_sync_id =1;
          Ctrl_Sync_send_pending_packets(&CTRL_SYNC_STR[1],1);
        }
        break;
              
    }
  }
  
}

void CTRL_sync_IRQ_Handler(){
    if(!Cinterval_CTRL_Started){
      Cinterval_CTRL_Started=1;
      BlueNRG_ConnInterval_IRQ_enable();
  }
}

