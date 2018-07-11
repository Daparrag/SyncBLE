/*media_interrup_c*/
#include "main.h"
#include "media_interrupt.h"

#ifdef DEBUG_MDA
#include "stm32f4xx_nucleo_add_led.h"
#endif

#define IRQn_SyncPresentationTime  TIM3_IRQHandler
static void MDA_error_Handler(void);
static uint8_t SyncPresentationTime_init = FALSE; 


/******************************************************************************/
/*

 _____ _                   _____       _                             _   
/__   (_)_ __ ___   ___    \_   \_ __ | |_ ___ _ __ _ __ _   _ _ __ | |_ 
  / /\/ | '_ ` _ \ / _ \    / /\/ '_ \| __/ _ \ '__| '__| | | | '_ \| __|
 / /  | | | | | | |  __/ /\/ /_ | | | | ||  __/ |  | |  | |_| | |_) | |_ 
 \/   |_|_| |_| |_|\___| \____/ |_| |_|\__\___|_|  |_|   \__,_| .__/ \__|
                                                              |_|        
 * TIM3 interrupt is used by the media synchronization service for allowing 
 * the streaming synchonization between multiple receivers. Therefore It is used 
 * mainly by recetors. 


*/


TIM_HandleTypeDef  TimHandle_service;
void TIM3_IRQHandler(void);
void (*presentation_func)(void);


/**
  * @brief  MDA_set_presentation_func once the sync time expire. 
* @param  remote_fun: function pointer
  * @retval void
  */
void MDA_set_presentation_func(void(*f)())
{
    presentation_func = f;

}

void MDA_interrupt_init(uint32_t period, uint32_t TickPriority){

TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  uint32_t              uwTimclock;
  uint32_t              uwPrescalerValue;
  uint32_t	        mperiod;
  //uint32_t              pFLatency

 /*Configure the TIM3 IRQ priority */
  HAL_NVIC_SetPriority(TIM3_IRQn, TickPriority ,0U);
  
  /* Enable the TIM3 global Interrupt */
  HAL_NVIC_EnableIRQ(TIM3_IRQn);
  
  /* Enable TIM3 clock */
  __TIM3_CLK_ENABLE();



  /*get_clock config*/
  //HAL_RCC_GetClockConfig(&clkconfig,&pFLatency);
  uwTimclock = 1 * HAL_RCC_GetPCLK2Freq(); /*Timer_Clocks*/
  uwPrescalerValue =(uint32_t) ((uwTimclock / 1000000) - 1);/*now the prescale counter clock is equal to 1Mhz*/
  mperiod = period  * ((1000000 / 1000) - 1); /*Period In ms*/ /*(1); period in us*/
  
  /* Initialize TIM3 */
  TimHandle_service.Instance = TIM3;
  
  TimHandle_service.Init.Prescaler = uwPrescalerValue;
  TimHandle_service.Init.ClockDivision = 0;
  TimHandle_service.Init.CounterMode = TIM_COUNTERMODE_UP;
  TimHandle_service.Init.Period = mperiod; 
  if(HAL_TIM_Base_Init(&TimHandle_service) != HAL_OK)
  {
    
    MDA_error_Handler();
    /* Start the TIM time Base generation in interrupt mode */
  }
  
   sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&TimHandle_service, &sClockSourceConfig) != HAL_OK)
  {
    MDA_error_Handler();
  }
  
   sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&TimHandle_service, &sMasterConfig) != HAL_OK)
  {
   MDA_error_Handler();
  }
  
  HAL_TIM_Base_Start_IT(&TimHandle_service);
  SyncPresentationTime_init = TRUE;
}


/**
  * @brief  This function disable the TIM3 timer. 
  *         The time source is configured  to have X-ms  time base with a dedicated 
  *         Tick interrupt priority. 
  * @param  void
  * @retval void
  */
void MDA_TIM_DISABLE(void){
  __HAL_TIM_DISABLE(&TimHandle_service);
}

/**
  * @brief  This function disable the TIM3 timer. 
  *         The time source is configured  to have X-ms  time base with a dedicated 
  *         Tick interrupt priority. 
  * @param  void
  * @retval void
  */
void MDA_TIM_ENABLE(void){
  __HAL_TIM_ENABLE(&TimHandle_service);
}





/**
  * @brief  This function disable the TIM3 interruption. 
  *         The time source is configured  to have X-ms  time base with a dedicated 
  *         Tick interrupt priority. 
  * @param  void
  * @retval void
  */

void MDA_interrupt_deinit(void){

	/*disable the interrupt*/
	__HAL_TIM_DISABLE_IT(&TimHandle_service,TIM_IT_UPDATE);
	/*disable the TIMER3 clock*/
	__TIM3_CLK_DISABLE();
	/*Interrupt_deinit*/
	HAL_NVIC_DisableIRQ(TIM3_IRQn);
}


/**
  * @brief  MDA_Suspend_interrupt.
  * @note   disabling TIM3 update interrupt.
  * @param  None
  * @retval None
  */
void MDA_interrupt_suspend(void)
{

  __HAL_TIM_DISABLE_IT(&TimHandle_service,TIM_IT_UPDATE);
}

/**
  * @brief  ptp_Resume_interrupt 
  * @note   Enabling TIM3 update interrupt.
  * @param  None
  * @retval None
  */
void MDA_interrupt_resume(void)
{
  /* Enable TIM2 Update interrupt */
  __HAL_TIM_ENABLE_IT(&TimHandle_service, TIM_IT_UPDATE);
}



/**
  * @brief  MDA_update_interrupt 
  * @note   update the parameters of the interruption.
  * @param  None
  * @retval None
  */
void MDA_update_interrupt(uint32_t period, uint32_t TickPriority)
{
  
  if(SyncPresentationTime_init != TRUE){
	/*disable the interrupt*/
	__HAL_TIM_DISABLE_IT(&TimHandle_service,TIM_IT_UPDATE);
	/*disable the TIMER3 clock*/
	__TIM3_CLK_DISABLE();
	/*Interrupt_deinit*/
	HAL_NVIC_DisableIRQ(TIM3_IRQn);
        MDA_interrupt_init(period, TickPriority);
  }else{
    
      	/*disable the interrupt*/
	__HAL_TIM_DISABLE_IT(&TimHandle_service,TIM_IT_UPDATE);
	/*disable the TIMER3 clock*/
	__TIM3_CLK_DISABLE();
	/*Interrupt_deinit*/
	HAL_NVIC_DisableIRQ(TIM3_IRQn);
        MDA_interrupt_init(period, TickPriority);
  }
	
        
#ifdef DEBUG_MDA
BSP_ADD_LED_On(ADD_LED2);        
#endif
}



void IRQn_StartSyncCallPresentation(){
  BSP_ADD_LED_On(ADD_LED2);  
}


void IRQn_SyncCallPresentation(){

#ifdef DEBUG_MDA
BSP_ADD_LED_Off(ADD_LED2);  
#else

  if(presentation_func != NULL){
      (*presentation_func)();    
  }
#endif

}


void IRQn_SyncPresentationTime(void){
IRQn_SyncCallPresentation(); 
HAL_TIM_IRQHandler(&TimHandle_service);
MDA_TIM_DISABLE();
//MDA_interrupt_suspend();
}  


void MDA_error_Handler(void)
{
  while(1);
}