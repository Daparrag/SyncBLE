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


typedef ctrl_status_table ctrl_status_entry;             /*used just to clarify*/

static ctrl_status_table CTRL_SYNC_STR[EXPECTED_NODES]; /*Synchronization control table*/


/**********************Static Func Def*************************/
static void Ctrl_Sync_error_handler(void);
static ctrl_status_entry * ctrl_get_status_table(chandler);
static uint8_t  parse_ctrl_sync_packet_header(uint8_t * data, uint8_t data_len, ctrl_sync_hdr * hdr);
static void process_init_packet(ctrl_init_packet * init_pack);
static ctrl_status_entry * ctrl_set_new_status_entry(chandler);
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
      ctrl_sync_tx_att.charProperties = CHAR_PROP_NOTIFY;
      ctrl_sync_tx_att.secPermissions = ATTR_PERMISSION_NONE;
      ctrl_sync_tx_att.gattEvtMask = GATT_DONT_NOTIFY_EVENTS;
      ctrl_sync_tx_att.encryKeySize=16;
      ctrl_sync_tx_att.isVariable=1;
      ret= APP_add_BLE_attr(&ctrl_sync_service,&ctrl_sync_tx_att);
    if(ret!=APP_SUCCESS)Ctrl_Sync_error_handler();
    /*INITIALIZE THE CONNECTION INTERVAL INTERRUPTION USED TO SEND DATA SYNCHRONOUSLY*/
     BlueNRG_ConnInterval_Init(10);
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
                            tClockTime arval_time);
{

      uint8_t ret;
      ctrl_sync_hdr ctrl_hdr;


/*enable notify*/
      if(attrhandler == ctrl_sync_tx_att.CharHandle + 2)
        {
               if(att_data[0] == 0x01){
                st->notify_enable=TRUE;
                }else{
                st->notify_enable=FALSE;
                }
        
/*process packet*/
        }else if(attrhandler == ctrl_sync_tx_att.Associate_CharHandler+1){
           /*parse the header*/
           ret = parse_ctrl_sync_packet_header(att_data,data_length,&ctrl_hdr);
           if(ret == 0) Ctrl_Sync_error_handler(); /*something is worng*/

           if(ctrl_hdr.pkt_type == INITIATOR){
                  ctrl_init_packet init_pack;
                  init_pack.header = ctrl_hdr;
                  ret += parse_ctrl_init_packet(att_data+ret, data_length-ret, &init_pack);
                  if (ret!=INIT_PCK_SIZE)Ctrl_Sync_error_handler(); /*something is worng*/

           }

        }


      //st = ctrl_get_status_table(chandler);
}


/**
  * @brief  This function returns the control table.
  * @param  uint16_t chandler : the connection handler defines the connection_id between pair devices.
  * @retval :  control table pointer.
  */

ctrl_status_entry *ctrl_get_status_table(uint16_t chandler){

ctrl_status_entry * ret = CTRL_SYNC_STR;

      while(ret != NULL)
      {
            if (ret->connect_id == chandler) break;
            ret++;

      }

return ret;
}




/**
  * @brief  This function returns a new control table entry or null.
  * @param  uint16_t chandler : serching parameter.
  * @retval :  ctrl_status_entry * : pointer to a control status entry.
  */
static ctrl_status_entry * ctrl_set_new_status_entry(chandler)
{


}


/**
  * @brief  This function parse and input control-sync init or report packet header.
  * @param  uint8_t * data : buffer input data.
  *   @param  uint8_t data_len : total length of the input data.
  *   @param  ctrl_sync_hdr * hdr : header data_structure.
  * @retval : payload pointer offset.
  */

static   uint8_t parse_ctrl_sync_packet_header(uint8_t * data, uint8_t data_len, ctrl_sync_hdr * hdr)
{
      uint8_t * p;

      if(data_len < 4)
      {
            /*something is wrong return 0*/
            return 0;
      }

      p=data;

      hdr->pkt_type =         p[0];
      hdr->connect_id =       ((p[1]<<8) | (p[2]));
      hdr->total_peers =      p[3];
      hdr->seq_id =           p[4];

      return 4;

}


/**
  * @brief  This function parse and input control-sync init packet header.
  * @param  uint8_t * data : buffer input data.
  *   @param  uint8_t data_len : total length of the input data.
  *   @param  ctrl_init_packet * ctrl_pack : init_data_structure.
  * @retval : payload pointer offset.
  */

static uint8_t  parse_ctrl_init_packet(uint8_t * data, uint8_t data_len,ctrl_init_packet * init_pack)
{
      uint8_t ret=0;
      uint16_t * p = data;

      init_pack->packet_count             = p[0];
      init_pack->tx_delay                 = p[1];
      init_pack->slave_max_delay_diff     = p[2];

      return 6;

}


/**
  * @brief  This function process a control sync init packet.
  * @param ctrl_init_packet * init_pack : input init packet to be processed;
  * @param uint16_t chandler : input init packet to be processed;
  * @retval : none.
  */

static void process_init_packet(ctrl_init_packet * init_pack, uint16_t chandler)
{
      ctrl_status_entry *st;

      st = ctrl_set_new_status_entry(chandler);
      if (st==NULL) return; /*wrong chandler or devices has not been correctly associated*/

      st->local_sync_status = IDLE;



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

