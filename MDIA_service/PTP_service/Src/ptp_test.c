/*this file brings suport for the packet creation/parse for the ptp protocol*/
/*Test file for ptp protocol*/

#ifdef BLE_APP_CONFIG
#include  <blefw_conf.h>
#endif

#include "ptp_test.h"
#include  "ble_firmware.h"
#include "app_ble.h"
#include "network.h"
#include "ptp_core.h"
/*choose properly*/
/*please not use this file with if you are testing the sync_ctrl_protocol*/
//MASTER//
#if defined( TEST_PTP_SERVER )
const uint8_t DEVICE_BDADDR[] =  { 0x55, 0x11, 0x07, 0x01, 0x16, 0xE1}; /*device addrs*/
const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','L','E','-','O','N','E',}; /*device name*/

/*specified address list */
uint8_t slaves_addreses[] = {PUBLIC_ADDR,0x55, 0x11, 0x07, 0x01, 0x16, 0xE3,
                             PUBLIC_ADDR,0x55, 0x11, 0x07, 0x01, 0x16, 0xE4
                            };

app_discovery_t DISC_config = {0x01 ,0x0020, 0x0020, 0x00,0x01,EXPECTED_NODES,slaves_addreses}; /*default configuration for the scan procedure*/                            



config_connection_t ConnSlave1 = {SCAN_P_S1, 
                                        SCAN_L_S1, 
                                        OUR_ADDRS_TYPE, 
                                        CONN_P1_S1, 
                                        CONN_P2_S1, 
                                        LATENCY_S1, 
                                        SUPERV_TIMEOUT_S1, 
                                        CONN_L1_S1, 
                                        CONN_L2_S1};/*connection default configuration*/

config_connection_t ConnSlave2 = {SCAN_P_S2, 
                                        SCAN_L_S2, 
                                        OUR_ADDRS_TYPE, 
                                        CONN_P1_S2, 
                                        CONN_P2_S2, 
                                        LATENCY_S2, 
                                        SUPERV_TIMEOUT_S2, 
                                        CONN_L1_S2, 
                                        CONN_L2_S2};/*connection default configuration*/

#elif defined(TEST_PTP_CLIENT)

//slave 1
//const uint8_t DEVICE_BDADDR[] =  { 0x55, 0x11, 0x07, 0x01, 0x16, 0xE2}; /*device addrs*/
//const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','L','E','-','T','W','O'}; /*device name*/                            


//slave 2
const uint8_t DEVICE_BDADDR[] =  { 0x55, 0x11, 0x07, 0x01, 0x16, 0xE3}; /*device addrs*/
const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','L','E','-','T','H','R','E','E'}; /*device name*/                            


//slave 3
//const uint8_t DEVICE_BDADDR[] =  { 0x55, 0x11, 0x07, 0x01, 0x16, 0xE4}; /*device addrs*/
//const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','L','E','-','F','O','U','R'}; /*device name*/                            
#endif

app_profile_t PROFILE;

 uint8_t test_create_header(uint8_t type, ptp_hdr * hdr,uint8_t * buff){
 	/*This test consist in the creation of a simple ptp_header 
	 * If everything is ok this function return a value !=0 otherwise 0
 	*/
 	uint8_t ret;
 	ret = create_ptp_packet_hdr(0,type,hdr,buff);

 	return ret;
 }

 uint8_t test_parse_header(ptp_hdr * hdr,uint8_t * data, uint8_t datalen){
 	/*This test consist parse a simple ptp_header  
	 * If everything is ok this function return a value !=0 otherwise 0
 	*/
 	uint8_t ret;
 	ret = ptp_packet_hdr_parse(data,datalen, hdr);
 	return ret;

 }

 uint8_t compare_headers(ptp_hdr * s1hdr,ptp_hdr * s2hdr){
 	/*This test consist compare two header data structures   
	 * If everything is ok this function return a value !=0 otherwise 0
 	*/
 	uint8_t ret=1;

 	if(s1hdr->ptp_type!=s1hdr->ptp_type)ret=0;
 	if(s1hdr->ptp_version!=s1hdr->ptp_version)ret=0;
 	if(s1hdr->domain_number!=s1hdr->domain_number)ret=0;
 	if(s1hdr->domain_number!=s1hdr->domain_number)ret=0;
 	if(s1hdr->control_field!=s1hdr->control_field)ret=0;
 	if(s1hdr->sequence_id!=s1hdr->sequence_id)ret=0;
 	if(s1hdr->msg_sync_interval!=s1hdr->msg_sync_interval)ret=0;
 	if(s1hdr->source_id!=s1hdr->source_id)ret=0;

 	return ret;


 }

 uint8_t test_header(){
 	/* This test is used to create and parse a ptp_header and
 	 * compare the results
 	 * * If everything is ok this function return a value !=0 otherwise 0
 	 */
uint8_t ret;
uint8_t tst_buff [5]; /*buffer set with a size of the header*/
ptp_hdr tst_ptphdr_cr; /*used to create the header */
ptp_hdr tst_ptphdr_pr;	/*used to parce the header */
 	/*1. create a sync_header*/
		ret = test_create_header(SYNC,&tst_ptphdr_cr,tst_buff);
		if (ret==0)return ret;
		ret=test_parse_header(&tst_ptphdr_pr,tst_buff,ret);
		if(ret==0)return ret;
		ret=compare_headers(&tst_ptphdr_cr,&tst_ptphdr_pr);
		if(ret==0)return ret;
return ret;		

 }



void test_ptp_server_application(void)
{
  /*  the ptp_server is who has the reference clock that will be used for synchonize 
   *  the ptp_client devices; for this solution was considered only one ptp_server  
   *  an has been designed as a device who begins the synchonization and who replay 
   *  the commands sent by the ptp_clients through the write request. 
   *  therefore the ptp_server has to scan for the service associated to the ptp_sync
   *  if the ptp_client devices doesn't replay for the scanning means that the ptp_client do not
   *  need to be synchonized therefore is discarded.
   *
   *  from the ptp_client side, it only has to wait for the sync command comming form the ptp_server
   *  then request properly through the notfication and finally wait for the ptp_server replay
   *  once all the iformaton needed for synchonize is collected the ptp_client ajust it local clock.  
   *  
   *  Once the clock in the ptp_client is ajusted; The device remain in this synchronous status for 
   *  a defined amount of time where once this is expired the device restart the  synchronization.  
   */
  
     static NET_Status ret_net;
      APP_Status ret_app;
/*1.0 set_address and name to the APP module*/ 
     ret_app = APP_Set_Address_And_Name_BLE(DEVICE_BDADDR,sizeof(DEVICE_BDADDR),local_name,sizeof(local_name));
     if(ret_app!=APP_SUCCESS)while(1);/*an error occur*/
     
/*1.1 initialize the device*/ 
     ret_app = APP_Init_BLE();
    if(ret_app!=APP_SUCCESS)while(1);/*an error occur*/
    
/*init_the_ptp_application*/
    Init_ptp_application(PTP_SERVER,&PROFILE); 
/*2. init the the network module*/
      network_t * network_config;      
      ret_net = init_network(NET_CONNECTED, DEVICE_CENTRAL,0x2,&network_config);
      if(ret_net != NET_SUCCESS)Error_Handler();  
      
/*3. associate this profile to both connections*/      
       uint8_t list_index [] = {0,1};
      ret_net = net_setup_profile_definition (&PROFILE,list_index,sizeof(list_index));
      if(ret_net!=NET_SUCCESS)while(1);/*an error occur*/
      
/*4. Configure an user discovery configuration*/  

      connection_handler_set_discovery_config(&DISC_config);
      //PTP_SYNC_set_periodic_sync(3600); 
      PTP_SYNC_set_periodic_sync(500);
      PTP_SYNC();

  do{
    network_process();
    HCI_Packet_Release_Event_CB();  
    
  }while(network_process() != DEVICE_READY);

      



      
      while(1)
      {
        //network_process();
        ptp_server_sync_process();
        HCI_Packet_Release_Event_CB();
      }
}



void test_ptp_client_application(void)
{
  
  /*  the ptp_server is who has the reference clock that will be used for synchonize 
   *  the ptp_client devices; for this solution was considered only one ptp_server  
   *  an has been designed as a device who begins the synchonization and who replay 
   *  the commands sent by the ptp_clients through the write request. 
   *  therefore the ptp_server has to scan for the service associated to the ptp_sync
   *  if the ptp_client devices doesn't replay for the scanning means that the ptp_client do not
   *  need to be synchonized therefore is discarded.
   *
   *  from the ptp_client side, it only has to wait for the sync command comming form the ptp_server
   *  then request properly through the notfication and finally wait for the ptp_server replay
   *  once all the iformaton needed for synchonize is collected the ptp_client ajust it local clock.  
   *  
   *  Once the clock in the ptp_client is ajusted; The device remain in this synchronous status for 
   *  a defined amount of time where once this is expired the device restart the  synchronization.  
   */
     NET_Status ret_net;
     APP_Status ret_app;
     //ptp_status_t ret_ptp;
/*1.0 set_address and name to the APP module*/ 
     ret_app = APP_Set_Address_And_Name_BLE(DEVICE_BDADDR,sizeof(DEVICE_BDADDR),local_name,sizeof(local_name));
     if(ret_app!=APP_SUCCESS)while(1);/*an error occur*/
     
/*2.0 initialize the device*/ 
     ret_app = APP_Init_BLE();
    if(ret_app!=APP_SUCCESS)while(1);/*an error occur*/
    
/*3.0 init_the_ptp_application*/
      Init_ptp_application(PTP_CLIENT,&PROFILE);
      //if(ret_ptp!=PTP_SUCESSS)while(1);/*an error occur*/
     
      
/*4.0 init the the network module*/
      network_t * network_config;      
      ret_net = init_network(NET_CONNECTED, DEVICE_PERISPHERAL,0x2,&network_config);
       if(ret_net != NET_SUCCESS)Error_Handler();  
      
      ret_net = net_setup_profile_definition (&PROFILE,NULL,0);/*sonce this is a PTP_CLIENT then it has only one master*/
      if(ret_net!=NET_SUCCESS)while(1);/*an error occur*/

/*5.0 for this specific test */  
      ret_net= service_handler_config(DONT_FIND_SERVICE,DONT_FIND_CHAR,NULL,0); /*ptp_client is who has to be synchonized. Therefore, for this solution is no needed to scan ptp_server services */
       if(ret_net!=NET_SUCCESS)while(1);/*an error occur*/
/*6.0 run the connection procedure until the connection 
and service discovery is sucessed*/
	do{
		network_process();
		HCI_Packet_Release_Event_CB();	

	}while(network_get_status() != 1);

	while(1)
	{
		ptp_client_sync_process();
		HCI_Packet_Release_Event_CB();
        }
 
}