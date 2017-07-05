/*Simple Sync-PTP BLE protocol*/

#ifndef PTP_BLE_H
#define PTP_BLE_H

#ifdef BLE_APP_CONFIG
#include  <blefw_conf.h>
#endif

#include  "ble_firmware.h"
#include <osal.h>

#define PTP_VERSION 0x0
#define SYNC_INTERVAL_MS 0x64 /*100ms*/

/*Messages_Type*/
#define SYNC 	  0x01
#define FOLLOW_UP 0x02
#define DELAY_REQ 0x04
#define DELAY_RSP 0x08

#define PTP_SERVER	0x00
#define PTP_CLIENT	0x01	


typedef enum 
{
	PTP_SUCESSS=0x00,
	PTP_ERROR=0x01
}ptp_status_t;

typedef enum 
{
        PTP_UNITIALIZED, 
        PTP_INIT, 
        PTP_WAIT_RESP, 
        PTP_PENDING_RESP
}ptp_state_t;

typedef enum
{
  PTP_UNSYNC, 
  PTP_SYNC 
}ptp_dv_status;

typedef struct
{
  tClockTime t0;
  tClockTime t1;
  tClockTime t2;
  tClockTime t3;
  
}ptp_clock_st;


typedef struct 
{
	uint16_t Chandler;
        ptp_dv_status dv_state;
	ptp_state_t ptp_state;
        uint8_t seq_number;
        uint8_t peer_ready_to_receive;
        ptp_clock_st timers;
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
	uint8_t source_id;	/*5  p[3] & P[8] unique connnection handler*/
}ptp_hdr;/*total 8bytes*/

typedef struct{
	ptp_hdr header; 
	 tClockTime orginTimeStamp;
}ptp_packet;





typedef struct ptp_fsm_ {
	ptp_state_t ptp_cstate;
	ptp_state_t ptp_substate;
}ptp_fsm;


/***********functions************/
uint8_t ptp_packet_hdr_parse(uint8_t * data,
                             uint8_t data_len, 
                             ptp_hdr * hdr);

uint8_t create_ptp_packet_hdr(uint16_t chndler,
                              uint8_t type, 
                              ptp_hdr * hdr,uint8_t *buff);


tBleStatus ptp_init_server();
void ptp_init_client();
void ptp_Dispatch(ptp_fsm * ptp_ins);

#endif /* PTP_BLE_H */