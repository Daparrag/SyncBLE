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


/*sync_control_interface*/

#ifndef _CTRL_CORE_H_
#define _CTRL_CORE_H_


#ifdef BLE_APP_CONFIG
#include  <blefw_conf.h>
#endif

#include  "ble_firmware.h"
#include <osal.h>
/*macro OP*/
#define ABS(x) (((x)<0) ? -(x):(x))




/*This configuration works initially only for two nodes estimated with the specification of <blefw_conf.h> */
#define IFS						(1.5)
#define PACKETS_SYNC_CINTERVAL 	 (4)
#define TX_INTERVAL 	 		 ((CONN_P1 + CONN_P2)/2)*1.25f
#define EXPECTED_DELAY			 (((CONN_L1 + CONN_L2)/2) * 0.625f +  IFS)


#ifdef USE_PTP_PROTOCOL
#define CTRL_MODE 		1 /*enable PTP to handler the control sync protocol*/
#else
#define CTRL_MODE  		0 /*use the static configuration to handler the control syn protocol*/
#endif





/**
  * @brief  This structure returns the control service operation outcome.
  */
typedef enum 
{
	CTRL_SUCCESS, 	/*Operation successful completed */
	CTRL_FAIL		/*Operation failed*/
}ctrl_sync_resp;

typedef enum 
{
  CTRL_STATIC_MODE,
  CTRL_DYNAMIC_MODE
}ctrl_mode;

/**
  * @brief  This struct defines the status of the control protocol.
  */
typedef enum
{
	UNSTARTED,								/*initial state the control serv has not been configured*/
	IDLE,							        /*device is initialized and waiting for synchonize command */
	SYNCRONIZING							/*device is synchronizing and waiting for respounse */
}ctrl_sync_status;


typedef struct
{
	uint16_t packet_count;					/*number of packets tramited or received*/
	uint16_t tx_delay;	/*cinterval*/		/*estimated TX delay from a source/sink devices*/
	uint16_t slave_max_delay_diff;			/* this define the maximum delay of a sink respect to the last device schedulled*/

}ctrl_sync_param;

#define CTRL_SYNC_PARM_SIZE		0x06 		/*control synchonization paramiters in bytes*/


/**
  * @brief  This struct defines the ctrl_table used by the synchronization protocol.
  */

typedef struct
{

	ctrl_sync_status  local_sync_status;  /*this is control link status*/
	uint16_t  connect_id;				  /*this value defines the connection_id between two devices*/
	uint8_t  seq_id;					  /*this is the packet seq_id*/
	uint8_t total_peers;			      /*number of peer that share the same type either source or sink*/
	ctrl_sync_param sync_param;
	uint8_t notify_enable;
	volatile uint8_t pending_pack;
	volatile uint8_t pending_pack_type;
}ctrl_status_entry;

typedef ctrl_status_entry ctrl_status_table;             /*used just to clarify*/

/**********CTRL Message Definition***********************/

/*Messages_Type*/
#define INITIATOR		0x01
#define REPORT_SRC 		0x02
#define REPORT_RRC 		0x04
/*static control protocol parameters*/
#define SOURCE_ID 		0x00




/**********SYNC_CONTROL_PACKET_DEF******/
/*header definition*/

typedef struct{
uint8_t pkt_type; 				/*8bits*/
uint16_t connect_id;			/*16bits*/
uint8_t total_peers;        	/*8bits*/
uint8_t  seq_id;
}ctrl_sync_hdr; /*4bytes*/
#define CTRL_HDR_PCK_SIZE		0x04	/*USED TO DEFINE THE SIZE OF THE CTRL PACKET HEADER IN BYTES*/
#define CTRL_INIT_PCK_PARAM		0x06	/*USED TO DEFINE THE SIZE OF THE INIT PACKET IN BYTES*/

typedef struct{
ctrl_sync_hdr header; 				/*4bytes*/
ctrl_sync_param  init_sync_parter;  /*6bytes*/
}ctrl_init_packet; /*10bytes*/

typedef ctrl_init_packet ctrl_src_report_packet; /*for this version i consider the same type but in theory it should be different*/

#define CTRL_INIT_PCK_SIZE		0x0A	/*USED TO DEFINE THE SIZE OF THE INIT PACKET IN BYTES*/
#define CTRL_RPC_PCK_SIZE		0x0A	/*USED TO DEFINE THE SIZE OF THE REPORT PACKET IN BYTES*/


/**
  * @brief  CTRL_get_control_table This function return a pointer to the control table.
  * @retval : ctrl_status_table *. pointer to the control table.
  *
  */

ctrl_status_table * CTRL_get_control_table(void);




/**
  * @brief  Ctrl_set_op_mode This function initializes the control synchronization protocol.
  * @param  app_profile_t * profile : profile to associate the service.
  * @retval : none.
  */

void Ctrl_set_op_mode(ctrl_mode op_mode);


/**
  * @brief  This function initializes the control synchronization protocol.
  * @param  app_profile_t * profile : profile to associate the service.
  * @retval : none.
  */

void Ctrl_Sync_init(app_profile_t * profile);



/**
  * @brief  This function process the input packets arriving form the BLE interface.
  * @param uint16_t chandler : connection handler associated ;
  *	@param ctrl_report_src_packet * src_report: src report data-structure
  *	@param uint8_t * buff : packet buffer
  * @retval : none.
  */
void Ctrl_input_packet_process(uint16_t chandler,
                            uint16_t attrhandler, 
                            uint8_t data_length, 
                            uint8_t *att_data, 
                            tClockTime arval_time);


/**
  * @brief  This function return the status of the synchronization protocol.
  * @param  uint16_t chandler connection handler that identified the peer device
  * @retval : control synchronization status.
  */

ctrl_sync_status  Ctrl_Get_sync_status(uint16_t chandler);


/**
  * @brief  This function return and specific remote device synchonization parmeters.
  * @param  uint16_t chandler connection handler that identified the peer device
  * @retval : control synchronization status.
  */

ctrl_sync_param * Ctrl_Get_remote_sync_param(uint16_t chandler);


/**
  * @brief  This function is used at the beginning of the media transmission
  			to sent the initialization parameters to all the synchronization-media-clients.
  * @param  uint8_t no_receivers : indicates the number of receivers.
  * @param  uint8_t no_packets : optional number of packets to transmit.
  * @retval : none
  */

void Ctrl_Sync_start(uint8_t no_peers, uint8_t no_packets);



/**
  * @brief  This function is to transmit synchronously ctrl packets using the connection interval
  			interrupt.
  * @param  : ctrl_status_entry * CTRL_SYNC_STR:  pointer to the entry for which will sent a message. 
  * @param  : uint8_t index :  index of the entre for which will sent a message. 
  * @retval : none
  */

void Ctrl_Sync_send_pending_packets( ctrl_status_entry * CTRL_SYNC_STR, uint8_t index);

/**
  * @brief  This function is used as the server main control process
  			interrupt.
  * @param  : none
  * @retval : none
  */

void Ctrl_Sync_server_main(void);


/**
  * @brief  This function is used as the client main control process
  			interrupt.
  * @param  : none
  * @retval : none
  */

void Ctrl_Sync_client_main(void);


/**
  * @brief  This function allows to sink devices enable the notify.
  * @param uint8_t chandler: peer connection handler;
  * @retval : (1) if the process is successed otherwise (0).
  */
static uint8_t CTRL_enable_notify(uint16_t chandler);


/**
  * @brief CTRL_sync: 
  *This function automatically send the control synchonization parameters
  * to all the peer devices.
  * @parm: none
  * @retval : none.
  */
void CTRL_sync();

/**
  * @brief CTRL_sync_send_sp_pk: 
  *This function send and specific packet type to all the peer devices
  * @parm: none
  * @retval : none.
  */
void CTRL_sync_send_sp_pk(uint8_t PK_type);



/**
  * @brief  This function deals to the connection interval Iterruption.
  * @retval : none.
  */
void Ctrl_Sync_cinterval_IRQ_handler(uint8_t connection_id);

/**
  * @brief CTRL_sync_IRQ_Handler: 
  * This Function is useful to raise the a flag to indocate a valid Cinterval 
  * @parm: none
  * @retval : none.
  */
void CTRL_sync_IRQ_Handler(void);



/**
  * @brief UPDATE_SYNC_IRQ
  *	This function deals with the updating parameters of the control sync protocol.
  * @retval : none.
  */
void UPDATE_SYNC_IRQ(void);


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
void CTRL_update_static_parameters(void);



#endif /* _CTRL_CORE_H_ */
