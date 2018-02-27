#ifndef PTP_CORE_H
#define PTP_CORE_H

#ifdef BLE_APP_CONFIG
#include  <blefw_conf.h>
#endif

#include  "ble_firmware.h"
#include <osal.h>

#define PTP_VERSION 0x0
#define PACKET_ALIVE_MS 0x64 /*200ms*/
/*Messages_Type*/
#define SYNC    0x01
#define FOLLOW_UP 0x02
#define DELAY_REQ 0x04
#define DELAY_RSP 0x08
/*Device Role*/
#define PTP_SERVER  0x00
#define PTP_CLIENT  0x01  

/*Protocol Status*/
typedef enum 
{
        PTP_UNITIALIZED, 
        PTP_INIT,
        PTP_FORWARD,
        PTP_WAIT_RESP, 
        PTP_PENDING_RESP,
        PTP_SYNCH
}ptp_state_t;

/*protocol acquisition_timers*/
typedef struct
{
  tClockTime t0;
  tClockTime t1;
  tClockTime t2;
  tClockTime t3;
}ptp_clock_st;

/*Protocol Control Table*/
typedef struct 
{
  uint16_t Chandler;
  ptp_state_t ptp_state;
  uint8_t pending_tx;
  uint8_t  node_id;
    uint8_t peer_device_address[6];            /*!< peer device address val*/
    uint8_t seq_number;
    uint8_t peer_ready_to_receive;
    uint8_t local_notify_enable_flag;
    ptp_clock_st timers;
    struct timer packet_alive;
}ptp_status_table;


/**********PTP_PACKET******/

/*header definition*/
typedef struct{
  uint8_t  ptp_type; /*4 bites*/
  uint8_t  ptp_version;/*1 bit*/
  uint8_t  domain_number; /*1 bit*/
  uint8_t  control_field; /*2 bit p[0]*/ 
  uint8_t  sequence_id; /*1 byte P[1]*/
  uint8_t  msg_sync_interval; /*P[2] 1byte*/
  uint16_t source_id; /*5  p[3] & P[8] unique connnection handler*/
}ptp_hdr;/*total 5bytes*/
#define PTP_HEADER_SIZE   5


/*ptp packet definition*/
typedef struct{
  ptp_hdr header; 
   tClockTime orginTimeStamp;
}ptp_packet;

/****************Functions*************************/	


/**
  * @brief init_ptp_profile: This function associate the ptp service to 
  * a profile
  * @param app_profile_t * profile: profile datastructure.
  * @
  */
void init_ptp_profile(app_profile_t * profile);

/**
  * @brief init_ptp_profile: This function initialize the ptp service structures 
  * and promitives. 
  * @param uint8_t no_peers: Number of devices associated to this service.
  * @
  */
void ptp_Start(uint8_t no_peers); /*initialize the ptp protocol*/


void Init_ptp_application(uint8_t ptp_dv_role, app_profile_t * profile);
void set_connection_clients(uint8_t conn_entries);
void set_connection_servers(uint8_t serv_conn_entries);
void ptp_server_sync_process();
void ptp_client_sync_process();
uint8_t ptp_send_ptp_packet(uint16_t chandler,uint8_t pkt_type,tClockTime * time_cpy);
void PTP_cinterval_IRQ_Handler(void);
void PTP_SYNC (void);
void PTP_SYNC_set_periodic_sync(uint32_t period);
void PTP_SYNC_enable_periodic_sync(void);
void PTP_SYNC_desable_periodic_sync(void);
void PTP_SYNC_update_periodic_sync(uint32_t period);
void PTP_SYNC_switch_off_periodic_sync(void);
void PTP_SYNC_IRQ_Handler();

/****Functions between PTP_SYNC and CTRL_SYNC*****/

/**
  * @brief PTP_Update_CTRL_Parameters_Callback: 
  * the synchonization Parameters at the Control Services.   
  * @return : none
  */
void PTP_Update_CTRL_Parameters_Callback(void);


#endif /*end PTP_CORE*/