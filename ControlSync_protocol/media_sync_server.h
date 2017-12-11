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


#ifdef USE_TIMESTAMP
#define CTRL_MODE 		1
#else
#define CTRL_MODE  		0
#endif

typedef enum 
{
	UNSTARTED,
	STARTING,
	SYNCRONIZING,
	IDLE
}ctrl_sync_status;


typedef struct {
uint8_t session_id;
uint8_t total_receivers;
uint8_t total_packets;
}streaming_session;


typedef struct {
uint8_t receiver_id;
streaming_session *  session_conf;
uint32_t tx_delay;
uint32_t * inter_slave_delay;
}str_ctrl_frame;




typedef struct {
uint8_t receiver_id;
uint8_t total_receivers;
uint8_t total_packets;
uint16_t tx_delay;
uint16_t inter_slave_delay;
}cstr_ctrl_sync;


void Ctrl_Sync_init(app_profile_t * profile);
ctrl_sync_status Ctrl_Sync_status(void);
void Ctrl_Sync_set_status(ctrl_sync_status status);
void Ctrl_Sync_start(uint8_t no_receivers, uint8_t no_packets);
void Ctrl_Sync_connection_interval_handler();



#endif /* _CTRL_SYNC_H_ */
