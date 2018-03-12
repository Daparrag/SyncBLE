#ifndef __MEDIA_SERVICE__
#define __MEDIA_SERVICE__

#include <stdio.h>
#include "hal_types.h"
#include "media_sync_server.h"
#include "ptp_core.h"
#include "network.h"



typedef enum{
STATIC_CONTROL,		/*The media service operating as STATIC_CONTROL mode uses the connection interval parameters */
					/*to stream the control information*/
DYNAMIC_CONTROL		/*The media service operating as DYNAMIC_CONTROL utilizes the PTP_Protocol to get the  */
					/*stream the control information before to send it to the synchonization set*/	
}sync_mode_t;


typedef enum{
MDIA_INIT,			/*multimedia service is ininialized*/
MDIA_DINIT,			/*multimedia service is not ininialized*/
MDIA_EABLE,			/*multimedia service is enable */
MDIA_DABLE			/*multimedia service is denable*/
}media_status_t;

typedef struct 
{
	uint8_t * address;	/*device_address*/
	struct group_address_t * next_address;	/*next_pair_address*/	
}group_address_t;

typedef struct
{
	uint8_t sync_group_id;
	uint8_t device_in_group_id;
	group_address_t  group_address;
}sync_group_t; /*this is to provices plurality to the sync tinking in the BLE6.x standard*/


typedef struct
{
	sync_mode_t sync_mode; /*mutimedia service configuration mode*/
	uint32_t sync_period;  /*mutimedia service synchonization period in case of DYNAMIC_CONTROL*/
	media_status_t mdea_status; /*current status of a multimedia services*/	
	sync_group_t * group_ref;
}media_service_t;

 struct media_ctrl_
{
	uint16_t *Chandler;
	ctrl_sync_param * node_parameters;
        struct media_ctrl_ *node_next_param;

};

typedef struct media_ctrl_ media_ctrl_parameters;


#define SYNC_GROUPS 	1 /*flag to control statically the number of synchonization groups*/

void MDIA_init_service(app_profile_t * profile, sync_mode_t mode);
void MDIA_dinit_service(void);
void MDIA_get_sync_parameters(media_ctrl_parameters * );
void MDIA_start_service(uint8_t npeers);
void MDIA_set_periodic_sync(uint32_t period);
void MDIA_dable_periodic_sync(void);
void MDIA_eable_periodic_sync(void);
void MDIA_server_main(void);
void Media_cinterval_IRQ_Handler (uint8_t connection_id);
void MDIA_run_synchonize();
/*no yet_implemented*/
void MDIA_get_media_sync_config(void);
void MDIA_dsble_service(void);







#endif /*__MEDIA_SERVICE__*/