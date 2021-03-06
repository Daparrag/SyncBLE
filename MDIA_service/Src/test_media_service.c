#include "test_media_service.h"
#include  "ptp_interrupt.h"
#include  "media_interrupt.h"
#include  "stm32f4xx_it.h"

#if defined( DEBUG_MDA )
#include "stm32f4xx_nucleo_add_led.h"
#endif

app_profile_t PROFILE;




#if defined( TEST_MEDIA_SERVER )
const uint8_t DEVICE_BDADDR[] =  { 0x55, 0x11, 0x07, 0x01, 0x16, 0xE1}; /*device addrs*/
const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','L','E','-','O','N','E',}; /*device name*/

#elif defined(TEST_MEDIA_CLIENT)

#if defined (SLAVE_1)
// slave 1
const uint8_t DEVICE_BDADDR[] =  { 0x55, 0x11, 0x07, 0x01, 0x16, 0xE2}; /*device addrs*/
const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','L','E','-','T','W','O'}; /*device name*/                            


#elif defined (SLAVE_2)
//slave 2
const uint8_t DEVICE_BDADDR[] =  { 0x55, 0x11, 0x07, 0x01, 0x16, 0xE3}; /*device addrs*/
const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','L','E','-','T','H','R','E','E'}; /*device name*/                            

#elif defined (SLAVE_3)
//slave 3
const uint8_t DEVICE_BDADDR[] =  { 0x55, 0x11, 0x07, 0x01, 0x16, 0xE4}; /*device addrs*/
const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','L','E','-','F','O','U','R'}; /*device name*/                            
#else 
#error "please chose properly you device ID before to use this test module. "
#endif

#endif


/*specified address list */
uint8_t slaves_addreses[] = {PUBLIC_ADDR,0x55, 0x11, 0x07, 0x01, 0x16, 0xE3,
                             PUBLIC_ADDR,0x55, 0x11, 0x07, 0x01, 0x16, 0xE4
                            };


app_discovery_t DISC_config = {0x01 ,0x0020, 0x0020, 0x00,0x01,EXPECTED_NODES,slaves_addreses}; /*default configuration for the scan procedure*/                            


void test_media_service(void){

	static NET_Status ret_net;
APP_Status ret_app;
/*1.0 set_address and name to the APP module*/ 
     ret_app = APP_Set_Address_And_Name_BLE(DEVICE_BDADDR,sizeof(DEVICE_BDADDR),local_name,sizeof(local_name));
     if(ret_app!=APP_SUCCESS)while(1);/*an error occur*/
     
/*1.1 initialize the device*/ 
     ret_app = APP_Init_BLE();
    if(ret_app!=APP_SUCCESS)while(1);/*an error occur*/

/*1.3 initialize the media_services as dynamic*/
     MDIA_init_service(&PROFILE,DYNAMIC_CONTROL);

/*2.0 init the the network module*/
     network_t * network_config; 
#if defined(TEST_MEDIA_SERVER)    
     ret_net = init_network(NET_CONNECTED, DEVICE_CENTRAL,0x2,&network_config);   
    if(ret_net != NET_SUCCESS)while(1);
#elif defined(TEST_MEDIA_CLIENT)    
      ret_net = init_network(NET_CONNECTED, DEVICE_PERISPHERAL,0x2,&network_config);
       if(ret_net != NET_SUCCESS)Error_Handler();
#endif       
      
/*3.0   
     server is the one who must scan for the control attributes associated to client-peer devices.*/                
    
       uint8_t list_index [] = {0,1};
#if defined(TEST_MEDIA_SERVER)       
      ret_net= service_handler_config(FIND_SPE_SERVICE,FIND_SPE_CHAR,list_index,2); /*ptp_client is who has to synchonize therefore for this solution is no needed to scan ptp_server services */
       if(ret_net!=NET_SUCCESS)while(1);/*an error occur*/
#elif defined(TEST_MEDIA_CLIENT)
	ret_net= service_handler_config(DONT_FIND_SERVICE,DONT_FIND_CHAR,NULL,0);
	 if(ret_net!=NET_SUCCESS)while(1);/*an error occur*/
#endif
/*4.0 
      associate this profile to both connections*/           
      ret_net = net_setup_profile_definition (&PROFILE,list_index,sizeof(list_index));
      if(ret_net!=NET_SUCCESS)while(1);/*an error occur*/
     connection_handler_set_discovery_config(&DISC_config);
     
 /*5.0
        run the connection procedure until the connection 
	and service discovery is sucessed*/
        
	do{
		network_process();
		HCI_Packet_Release_Event_CB();	

	}while(network_get_status() != 1);

/*6.0 
        lets start the MDIA Service.*/
	uint8_t receivers = NET_get_num_connections();
        MDIA_start_service(receivers);
        
/*7.0*/
     /*initialize and enable the connection interval interruption*/
        BlueNRG_ConnInterval_Init(10);
	BlueNRG_ConnInterval_IRQ_enable();
        
        
        
        
/*8.0 init a LED in case of test*/
#if defined( DEBUG_MDA ) && defined(TEST_MEDIA_CLIENT)
BSP_ADD_LED_Init(ADD_LED2);
BSP_ADD_LED_Off(ADD_LED2);
#endif       
        
/*9.0*/
    /*lets everything works automatically*/
#if defined(TEST_MEDIA_SERVER) 
   MDIA_set_periodic_sync(5000);
   MDIA_eable_periodic_sync();
   
#endif

   while(1){
#if defined(TEST_MEDIA_SERVER)     
   MDIA_server_main();
#elif defined(TEST_MEDIA_CLIENT)
   MDIA_client_main();
#endif
   HCI_Packet_Release_Event_CB();
   }	
          
}