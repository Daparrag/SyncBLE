/* Sync_server_test_file
*
* This file include the test for the control protocol used to synchonize multiple 
* media client to a single media stream or simple data stream.  
*
*/


#ifdef BLE_APP_CONFIG
#include  <blefw_conf.h>
#endif
#include  "ble_firmware.h"
#include "app_ble.h"
#include "network.h"
#include "sync_ctrl_test.h"


/*choose properly*/
//MASTER//

app_profile_t PROFILE;

const uint8_t DEVICE_BDADDR[] =  { 0x55, 0x11, 0x07, 0x01, 0x16, 0xE1}; /*device addrs*/
const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','L','E','-','O','N','E',}; /*device name*/

//slave 1
//const uint8_t DEVICE_BDADDR[] =  { 0x55, 0x11, 0x07, 0x01, 0x16, 0xE2}; /*device addrs*/
//const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','L','E','-','T','W','O'}; /*device name*/                            


//slave 2
//const uint8_t DEVICE_BDADDR[] =  { 0x55, 0x11, 0x07, 0x01, 0x16, 0xE3}; /*device addrs*/
//const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','L','E','-','T','H','R','E','E'}; /*device name*/                            


//slave 3
//const uint8_t DEVICE_BDADDR[] =  { 0x55, 0x11, 0x07, 0x01, 0x16, 0xE4}; /*device addrs*/
//const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','L','E','-','F','O','U','R'}; /*device name*/                            



/*specified address list */
uint8_t slaves_addreses[] = {PUBLIC_ADDR,0x55, 0x11, 0x07, 0x01, 0x16, 0xE3,
                             PUBLIC_ADDR,0x55, 0x11, 0x07, 0x01, 0x16, 0xE4
                            };

app_discovery_t DISC_config = {0x01 ,0x0020, 0x0020, 0x00,0x01,EXPECTED_NODES,slaves_addreses}; /*default configuration for the scan procedure*/                            

void ctrl_sync_test_server()
{

/* 
 * this is the control media synchonization process used to indicate a set of clients the configuration parameters
 * used to interdestination synchronization. Each application have to ajust the presentation time according to this parameters
 * independent of this protocol.
 * 
 *
*/
static NET_Status ret_net;
APP_Status ret_app;
/*1.0 set_address and name to the APP module*/ 
     ret_app = APP_Set_Address_And_Name_BLE(DEVICE_BDADDR,sizeof(DEVICE_BDADDR),local_name,sizeof(local_name));
     if(ret_app!=APP_SUCCESS)while(1);/*an error occur*/
     
/*1.1 initialize the device*/ 
     ret_app = APP_Init_BLE();
    if(ret_app!=APP_SUCCESS)while(1);/*an error occur*/

/*1.1 initialize the control_sync_application*/
     Init_Ctrl_Sync_application(&PROFILE);
/*2. init the the network module*/
  	       network_t * network_config;
  	       ret_net = init_network(NET_CONNECTED, DEVICE_CENTRAL,0x2,&network_config);   
  	        if(ret_net != NET_SUCCESS)Error_Handler();

/*3. associate this profile to both connections*/           
       uint8_t list_index [] = {0,1};
      ret_net = net_setup_profile_definition (&PROFILE,list_index,sizeof(list_index));
      if(ret_net!=NET_SUCCESS)while(1);/*an error occur*/
     connection_handler_set_discovery_config(&DISC_config);

/*4. Since for this test we only need the to indcate an static configuration parameters form the 
 *	 media-synchonization-server we could avoid discover services and characteristics at the server 
 *   that contain the data to be used by the clients.
 *
 * in this case  of uses a ptp-prtocol to synchonize the clock of the  clients you must to avoid this line  

*/
#if (!CTRL_MODE)
	ret_net= service_handler_config(DONT_FIND_SERVICE,DONT_FIND_CHAR,NULL,0);
	 if(ret_net!=NET_SUCCESS)while(1);/*an error occur*/
#endif 

/*5 run the connection procedure until the connection 
	and service discovery is sucessed*/
	do{
		network_process();
		HCI_Packet_Release_Event_CB();	

	}while(network_process() != DEVICE_READY)


/*5. lets start the control sync_process at the media synchonization server*/
	uint8_t receivers = NET_get_num_connections();	
/*6. initialize the connection interval interruption*/
	BlueNRG_ConnInterval_IRQ_enable();

/*7. send the initiator packet to the slaves before to stream data */
	Ctrl_Sync_start(receivers,0); /*this will forward the packets according to the connection interval*/

/*8. send a report packet to the clients this could be send during the normal execution of 
 the stream data process*/	
}



void ctrl_sync_test_client()
{

/* 
 * this is the control media synchonization process used to receive the configuration parameters used to interdestination synchronization. 
 * Each application have to ajust the presentation time according to this parameters independent of this protocol.
 *
*/
	 static NET_Status ret_net;
	  APP_Status ret_app;
	  /*1.0 set_address and name to the APP module*/
	  ret_app = APP_Set_Address_And_Name_BLE(DEVICE_BDADDR,sizeof(DEVICE_BDADDR),local_name,sizeof(local_name));
     if(ret_app!=APP_SUCCESS)while(1);/*an error occur*/
	  /*2.0 initialize the device*/ 
     ret_app = APP_Init_BLE();
    if(ret_app!=APP_SUCCESS)while(1);/*an error occur*/

    /*1.1 initialize the control_sync_application*/
     Init_Ctrl_Sync_application(&PROFILE);

     /*4.0 init the the network module*/
     
     

}