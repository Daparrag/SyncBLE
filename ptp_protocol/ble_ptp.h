/*Simple Sync-PTP BLE protocol*/

#ifndef PTP_BLE_H
#define PTP_BLE_H

#ifdef BLE_APP_CONFIG
#include  <blefw_conf.h>
#endif

#include  "ble_firmware.h"


/*Messages_Type*/
#define SYNC 	  0x01
#define FOLLOW_UP 0x02
#define DELAY_REQ 0x04
#define DELAY_RSP 0x08

#define PTP_MASTER	0x00
#define PTP_SLAVE	0x01	


typedef enum 
{
	PTP_SUCESSS=0x00,
	PTP_ERROR=0x01
}ptp_status_t;

typedef enum 
{
        PTP_UNITIALIZED, 
        PTP_INIT, 
        PTP_UNSYNC, 
        PTP_SYNC, 
        PTP_WAIT_RESP, 
        PTP_PENDING_REQ
}ptp_state_t;



typedef struct 
{
	uint16_t Chandler;
	ptp_state_t state;
}ptp_status_table;


/** 
* @brief ptp aprox packet structures.
*/
typedef struct{
	uint8_t  ptp_type; /*4 bites*/
	uint8_t  ptp_version;/*1 bit*/
	uint8_t  domain_number; /*1 bit*/
	uint8_t  control_field; /*2 bit p[0]*/ 
	uint8_t  sequence_id; /*1 byte P[1]*/
	uint8_t  msg_sync_interval; /*P[2] 1byte*/
	uint8_t source_id;	/*5  p[3] & P[8]*/
}ptp_hdr;/*total 8bytes*/

typedef struct{
	ptp_hdr header; 
	uint16_t orginTimeStamp;
}ptp_packet;





typedef struct ptp_fsm_ {
	ptp_state_t ptp_cstate;
	ptp_state_t ptp_substate;
}ptp_fsm;

tBleStatus ptp_init_server();
void ptp_init_client();
void ptp_Dispatch(ptp_fsm * ptp_ins);

#endif /* PTP_BLE_H */