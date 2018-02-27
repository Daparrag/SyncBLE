

#include "media_service.h"
#include <stdlib.h>

media_service_t CTRL_TABLE [SYNC_GROUPS];
static media_status_t service_status = MDIA_DINIT;

/*****************static-func******************/
static media_service_t * get_free_mdia_entry(void);

/**********************************************/



media_service_t * get_free_mdia_entry(void) 
{
return 0;
}



/**
  * @brief  Ctrl_Sync_init: 
  * This function initializes media synchonization service components.
  * @param  app_profile_t * profile : profile to associate the subservices.
  * @param  sync_mode_t : operational mode Static or Dynamic.
  * @retval : none.
  */

void MDIA_init_service(app_profile_t * profile, sync_mode_t mode)
{	
	if (service_status != MDIA_DINIT)return;
	if(mode == DYNAMIC_CONTROL)
		Ctrl_set_op_mode(CTRL_DYNAMIC_MODE);
/* 1.0 associate control and/or the ptp service to a profile*/
	Ctrl_Sync_init(profile);
	init_ptp_profile(profile);
	
	service_status = MDIA_INIT;	

}


void MDIA_dsble_service()
{
	
}

void MDIA_deinit_service()
{
	PTP_SYNC_desable_periodic_sync();
}


/**
  * @brief  MDIA_start_service: 
  * This function starts the media synchonization service components.
  * @param  uint8_t npeers.
  * @retval : none.
  */

void MDIA_start_service(uint8_t npeers){
	/* 1.0 Start the control synchronization protocol and/or the ptp_protocol */
	if(service_status != MDIA_INIT  || !network_get_status())return;
	Ctrl_Sync_start(npeers,0);
	service_status = MDIA_EABLE;	
}



void MDIA_get_sync_parameters(media_ctrl_parameters * tmp_sync_parm)
{
    uint8_t i;
    media_ctrl_parameters ** tmp_ret = &tmp_sync_parm;
    ctrl_status_table * tmp_tbl;
    
    tmp_tbl = CTRL_get_control_table();
    i= tmp_tbl->total_peers;

    do
    {
    	(*tmp_ret)->Chandler = (uint16_t *)&(tmp_tbl->connect_id);
    	(*tmp_ret)->node_parameters = (ctrl_sync_param*)&(tmp_tbl->sync_param);
     	(*tmp_ret)->node_next_param = (struct media_ctrl_ *) malloc(1* sizeof(struct media_ctrl_));
     	 tmp_ret = &((*tmp_ret)->node_next_param);

    }while(i!=0);

}


/**
  * @brief  MDIA_set_periodic_sync: 
  * This function initialize a periodic Interruption to run the synchronization periodically.
  * @param  uint16_t period: Synchronization Period (ms).
  * @ return: none
  */

void MDIA_set_periodic_sync(uint32_t period)
{
  PTP_SYNC_set_periodic_sync (period);
}


/**
  * @brief MDIA_dable_periodic_sync:
  * This function disable the periodic synchronization interruption.
  * @ return : none
  */
void MDIA_dable_periodic_sync()
{
	PTP_SYNC_desable_periodic_sync();
}


/**
  * @brief MDIA_update_periodic_sync:
  * This function  update the synchronization period.
  * @param uint32_t period: synchronization period
  * @return : none
  */
void MDIA_update_periodic_sync(uint32_t period){
PTP_SYNC_update_periodic_sync(period);	
}


/**
  * @brief MDIA_eable_periodic_sync:
  * This function  is used to enable the periodic sync.
  * @return : none
  */
void MDIA_eable_periodic_sync()
{
	PTP_SYNC_enable_periodic_sync();
}


