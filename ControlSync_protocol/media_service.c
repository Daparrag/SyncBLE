

#include "media_service.h"
#include <stdlib.h>

media_service_t CTRL_TABLE [SYNC_GROUPS];
static media_status_t service_status = MDIA_DINIT;
static sync_mode_t mdia_mode = STATIC_CONTROL; /*default control mode*/
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
	mdia_mode = mode;
/* 1.0 associate control and/or the ptp service to a profile*/
   Ctrl_Sync_init(profile);
    if(mode == DYNAMIC_CONTROL){
		Ctrl_set_op_mode(CTRL_DYNAMIC_MODE);	
		init_ptp_profile(profile);
	}

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
	if(mdia_mode == DYNAMIC_CONTROL) ptp_Start(npeers);
	service_status = MDIA_EABLE;
/* the initialization an configuration of the connection interval is delegated to the application */		
}



/**
  * @brief  MDIA_run_synchonize: 
  * This function send the synchonization signal to all the peer devices.
  * @param:   none.
  * @retval : none.
  */
void MDIA_run_synchonize(){
	/*this function send the synchonization signal to all the peer devices*/
	/**/
		//CTRL_sync();
		if(mdia_mode == DYNAMIC_CONTROL)
		PTP_SYNC();
}



/**
  * @brief  MDIA_get_sync_parameters: 
  * This function return the sync parameters to be used by the API.
  * @param  uint8_t npeers.
  * @retval : none.
  */
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
  * @brief  MDIA_server_main: 
  * This function is the main process of the synchronization framework.
  * @param  uint8_t npeers.
  * @retval : none.
  */
void MDIA_server_main(){
	Ctrl_Sync_server_main();
if(mdia_mode == DYNAMIC_CONTROL)	
	ptp_server_sync_process_temp_new();
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

void Media_cinterval_IRQ_Handler (uint8_t connection_id){
  if(mdia_mode == DYNAMIC_CONTROL) PTP_cinterval_IRQ_Handler_idx(connection_id);
  Ctrl_Sync_cinterval_IRQ_handler(connection_id);
}
  

