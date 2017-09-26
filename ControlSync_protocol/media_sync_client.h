/*sync_control_interface*/

#ifndef _CTRL_SYNC_H_
#define _CTRL_SYNC_H_


#ifdef BLE_APP_CONFIG
#include  <blefw_conf.h>
#endif

#include  "ble_firmware.h"
#include <osal.h>

/*This configuration works initially only for two nodes estimated with the specification of <blefw_conf.h> */
#define IFS						(1.5)
#define PACKETS_SYNC_CINTERVAL 	 (4)
#define TX_INTERVAL 	 		 ((CONN_P1 + CONN_P2)/2)
#define EXPECTED_DELAY			 (((CONN_L1 + CONN_L2)/2) +  IFS)


#ifdef USE_PTP_PROTOCOL
#define CTRL_MODE 		1 /*enable PTP to handler the control sync protocol*/
#else
#define CTRL_MODE  		0 /*use the static configuration to handler the control syn protocol*/
#endif




typedef enum 
{
	UNSTARTED,
	STARTING,
	SYNCRONIZING,
	IDLE
}ctrl_sync_status;




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
uint8_t source_id;				/*8bits*/
uint8_t receiver_id;				/*8bits*/
uint8_t total_receivers;                        /*8bits*/
uint8_t  seq_id;
}ctrl_sync_hdr; /*4bytes*/


typedef struct{
ctrl_sync_hdr header; 				/*4bytes*/
uint8_t total_packets;				//8bits// //optonal//
uint16_t tx_delay;					//16bits//	
uint16_t slave_max_delay;			//16bits//	  
}ctrl_init_packet; /*9bytes*/


typedef struct{
ctrl_sync_hdr header; 
uint16_t tx_delay;					//16bits//	
uint16_t slave_max_delay;			//16bits//	
}ctrl_report_src_packet;/*8bytes*/



/* Report RRC_packet not yet implemented
typedef struct{
ctrl_sync_hdr header; 
uint16_t tx_delay;					//16bits//	
uint16_t slave_max_delay;			//16bits//	
}report_rrc_packet;
*/


/*Protocol Control Table*/

typedef struct {
uint8_t session_id;
uint8_t total_receivers;
uint8_t total_packets;
}streaming_session;

typedef struct 
{
#if	(ROLE == GAP_PERIPHERAL_ROLE)
	ctrl_sync_status  local_sync_status; //used to support multiple source streams (not yet implemented)
#endif	
	uint8_t  source_id;
	uint8_t  receiver_id;
	uint8_t  seq_id;
	uint8_t total_receivers;
	uint8_t total_packets;
	uint16_t tx_delay;
	uint16_t slave_max_delay_diff;			//16bits//
}ctrl_status_table;




void Ctrl_Sync_init(app_profile_t * profile);

void ctrl_input_packet_process(uint16_t chandler, 
                            uint16_t attrhandler, 
                            uint8_t data_length, 
                            uint8_t *att_data, 
                            tClockTime arval_time);

ctrl_sync_status Get_Ctrl_Sync_status(void);

void Ctrl_Sync_start(uint8_t no_receivers, uint8_t no_packets);

void Ctrl_Sync_send_pending_packets(void);

void Ctrl_Sync_client_process(void);







/************OLD**************/




typedef struct {
uint8_t receiver_id;
streaming_session *  session_conf;
uint32_t tx_delay;
uint32_t * inter_slave_delay;
}str_ctrl_frame;


typedef struct {
uint8_t receiver_id;
uint16_t seq_id;
uint8_t total_receivers;
uint8_t total_packets;
uint16_t tx_delay;
uint16_t inter_slave_delay;
}ctrl_sync_table;





#endif /* _CTRL_SYNC_H_ */
