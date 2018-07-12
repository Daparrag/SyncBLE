

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
#if defined(USE_ONLY_PTP)

  PTP_SET_operation_mode(PTP_DYNAMIC);
  init_ptp_profile(profile);
  
#else 
	if (service_status != MDIA_DINIT)return;
        
        mdia_mode = mode;
        Ctrl_Sync_init(profile);
        
        if(mode == DYNAMIC_CONTROL)
        {
          Ctrl_set_op_mode(CTRL_DYNAMIC_MODE);
          PTP_SET_operation_mode(PTP_DYNAMIC);
          init_ptp_profile(profile);
          
        }
#endif        
        service_status = MDIA_INIT;	
             
/*init the TIM4 as clock */
clock_interrupt_init(6);
//MDA_update_interrupt(5,6);
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
#if defined(USE_ONLY_PTP)
  ptp_Start(npeers);
#else
	/* 1.0 Start the control synchronization protocol and/or the ptp_protocol */
	if(service_status != MDIA_INIT  || !network_get_status())return;
	Ctrl_Sync_start(npeers,0);
	if(mdia_mode == DYNAMIC_CONTROL) ptp_Start(npeers);
#endif	
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
#if defined(USE_ONLY_PTP)  
   PTP_SYNC();
#else
  if(mdia_mode == DYNAMIC_CONTROL)
  {
    PTP_SYNC();
  }else if (mdia_mode == STATIC_CONTROL)
  {
    CTRL_sync();
  
  }
#endif		
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
#if defined(USE_ONLY_PTP)
  ptp_server_sync_process_temp_new();
#else  
	Ctrl_Sync_server_main();
if(mdia_mode == DYNAMIC_CONTROL)	
	ptp_server_sync_process_temp_new();
#endif
}

/**
  * @brief  MDIA_client_main: 
  * This function is the main process of the synchronization framework(clients).
  * @param  uint8_t npeers.
  * @retval : none.
  */

void MDIA_client_main(){
#if defined(USE_ONLY_PTP)  
  ptp_client_sync_process_new_tmp();
#else    
	Ctrl_Sync_client_main();
      
if(mdia_mode == DYNAMIC_CONTROL)	
	ptp_client_sync_process_new_tmp();
#endif
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


/**
  * @brief MDIA_eable_periodic_sync:
  * This function  is used to enable the periodic sync.
  * @return : none
  */
void Media_cinterval_IRQ_Handler (uint8_t connection_id){
#if defined(USE_ONLY_PTP)
  PTP_cinterval_IRQ_Handler_idx(connection_id);
#else  
  Ctrl_Sync_cinterval_IRQ_handler(connection_id);
  if(mdia_mode == DYNAMIC_CONTROL) PTP_cinterval_IRQ_Handler_idx(connection_id);
#endif
}
  

/**
  * @brief MDIA_init_stream_IRQn:
* This function  is used to enable a timer for the IRQn_stream interrupt.
  * @return : none
  */

void MDIA_init_stream_IRQn(){

  MDA_interrupt_init(1500, 6); /*this is a random initialization of 1.5ms or (1500us)*/
  MDA_interrupt_suspend();
}


/**
  * @brief MDIA_enable_stream_IRQn:
* This function  is used to enable a timer for the IRQn_stream interrupt.
  * @return : none
  */
void MDIA_enable_stream_IRQn()
{

  MDA_interrupt_resume();

}

/**
  * @brief MDIA_disable_stream_IRQn:
* This function  is used to enable a timer for the IRQn_stream interrupt.
  * @return : none
  */
void MDIA_disable_stream_IRQn()
{
   MDA_interrupt_suspend();
}



/**
  * @brief MDIA_ready_command:
* This function  to start a timer before to stream the data.
  * @return : none
  */
void MDIA_ready_cmd(){
  
    ctrl_status_table * tmp_ctrl_table = CTRL_get_control_table();
     if( tmp_ctrl_table->seq_id == 1){
         /*connection 1 is the lastone in receive data therefore it do not need to wait*/
       
        //uint32_t period = (uint32_t)(tmp_ctrl_table->sync_param.slave_max_delay_diff +1.5);
          IRQn_SyncCallPresentation();
     }else if(tmp_ctrl_table->seq_id == 2){
        //Connection 2 will be sync to conneciton 1.
       uint32_t period = (uint32_t)(tmp_ctrl_table->sync_param.slave_max_delay_diff +2.5);
        MDA_update_interrupt(period,6);
       
     }

  
}


  
void MEDIA_set_callbackSyncFunction(void (*f) ()){
  
  MDA_set_presentation_func(f);/*here it is necessary to assign a function to call once the interrupt is raise*/
}




/**
  * @brief MDIA_set_stream_IRQn:
* This function  is used to setup the stream_IRQ.
  * @return : none
  */
void MDIA_set_stream_IRQn(uint32_t period,remote_fun stream_fun){
  MDA_set_presentation_func(stream_fun);/*here it is necessary to assign a function to call once the interrupt is raise*/
  MDA_update_interrupt(period,6);
  
}


/**
  * @brief MEDIA_get_mode:
  * This function returns the configuration mode : static or dynamic.
  * @return : none
  */

sync_mode_t MEDIA_get_mode(void){
  return mdia_mode;

}

/**
  * @brief MEDIA_update_static_parameters:
  * This function call the RTCP protocol to update the static paramiters 
  * when there is not used the default connection parameters.
  * @return : none
  */

void MEDIA_update_static_parameters(void)
{
  CTRL_update_static_parameters();
}

