

 #include "test_media_service.h"


app_profile_t PROFILE;


#define TEST_SERVER

#if defined( TEST_SERVER )
const uint8_t DEVICE_BDADDR[] =  { 0x55, 0x11, 0x07, 0x01, 0x16, 0xE1}; /*device addrs*/
const char local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','L','E','-','O','N','E',}; /*device name*/

#elif defined(TEST_CLIENT)

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
     
}