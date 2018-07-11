#ifndef PTP_TEST_H
#define PTP_TEST_H

#include "ptp_core.h"
#include  "ble_firmware.h"
#include "app_ble.h"
#include "network.h"

//#define TEST_SERVER

#if !defined(TEST_PTP_SERVER) && !defined(TEST_PTP_CLIENT)
#error "you should define TEST_PTP_SERVER or TEST_PTP_CLIENT to use this module"
#endif


/*I dont know why i put it ... :o*/
/*Config Slave 1*/
#define SCAN_P_S1  						(0x0028)      		  /*!< Scan Interval 40ms. >*/			
#define SCAN_L_S1  						(0x0028)      		  /*!< Scan Window.  20ms  >*/
#define CONN_P1_S1 			 ((int)((24)/1.25f))  	          /*!< Min connection interval in ms. >*/
#define CONN_P2_S1				 ((int)((24)/1.25f))  	          /*!< Max connection interval in ms. >*/
#define SUPERV_TIMEOUT_S1  				  (3200)         	  /*!<  Supervision timeout.   >*/
#define CONN_L1_S1         	((int)((10)/0.625f))   	   		  /*!<  Min connection length. >*/
#define CONN_L2_S1         	((int)((10)/0.625f))   	   		  /*!<  Max connection length. >*/
#define LATENCY_S1								 (0)

/*Config Slave 2*/

#define SCAN_P_S2  						(0x0028)      		  /*!< Scan Interval 40ms. >*/			
#define SCAN_L_S2  						(0x0028)      		  /*!< Scan Window.  20ms  >*/
#define CONN_P1_S2 			 ((int)((10)/1.25f))  	          /*!< Min connection interval in ms. >*/
#define CONN_P2_S2				 ((int)((24)/1.25f))  	          /*!< Max connection interval in ms. >*/
#define SUPERV_TIMEOUT_S2  				  (3200)         	  /*!<  Supervision timeout.   >*/
#define CONN_L1_S2         	((int)((10)/0.625f))   	   		  /*!<  Min connection length. >*/
#define CONN_L2_S2         	((int)((10)/0.625f))   	   		  /*!<  Max connection length. >*/
#define LATENCY_S2								 (0)





uint8_t test_create_header(uint8_t type, ptp_hdr * hdr,uint8_t * buff);
uint8_t test_parse_header(ptp_hdr * hdr,uint8_t * data, uint8_t datalen);
uint8_t compare_headers(ptp_hdr * s1hdr,ptp_hdr * s2hdr);
uint8_t test_header(void);


void test_ptp_server_application(void);
void test_ptp_client_application(void);




#endif /*ptp_test*/