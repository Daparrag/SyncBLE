/*ptp_suport interrupt file*/

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_tim_ex.h"
#include "clock_interface.h"
#include "media_service.h"
#include "ptp_interrupt.h"
#include "stm32f4xx_nucleo_add_led.h"
#ifdef DEBUG
#include "debug.h"
#endif




static void _Error_Handler();


/******************************************************************************/
/*

 _____ _                   _____       _                             _   
/__   (_)_ __ ___   ___    \_   \_ __ | |_ ___ _ __ _ __ _   _ _ __ | |_ 
  / /\/ | '_ ` _ \ / _ \    / /\/ '_ \| __/ _ \ '__| '__| | | | '_ \| __|
 / /  | | | | | | |  __/ /\/ /_ | | | | ||  __/ |  | |  | |_| | |_) | |_ 
 \/   |_|_| |_| |_|\___| \____/ |_| |_|\__\___|_|  |_|   \__,_| .__/ \__|
                                                              |_|        


*/


TIM_HandleTypeDef  TimHandle;
void TIM2_IRQHandler(void);


/**
  * @brief  This function configures the TIM2. 
  *         The time source is configured  to have X-ms  time base with a dedicated 
  *         Tick interrupt priority. 
  * @param  uint16_t period: Tick interrupt period (ms).
  * @retval void
  */

void ptp_interrupt_init(uint32_t period, uint32_t TickPriority){

  //RCC_ClkInitTypeDef    clkconfig;
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  uint32_t              uwTimclock;
  uint32_t              uwPrescalerValue;
  uint32_t	        mperiod;
  //uint32_t              pFLatency

 /*Configure the TIM2 IRQ priority */
  HAL_NVIC_SetPriority(TIM2_IRQn, TickPriority ,0U);
  
  /* Enable the TIM2 global Interrupt */
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
  
  /* Enable TIM2 clock */
  __TIM2_CLK_ENABLE();



  /*get_clock config*/
  //HAL_RCC_GetClockConfig(&clkconfig,&pFLatency);
  uwTimclock = 1 * HAL_RCC_GetPCLK2Freq(); /*Timer_Clocks*/
  uwPrescalerValue = (uint32_t) ((uwTimclock/1000000)-1);/*now the prescale counter clock is equal to 1Mhz*/
  mperiod = period * ((1000000/1000)-1); /*period in ms*/
  
  /* Initialize TIM2 */
  TimHandle.Instance = TIM2;
  
  TimHandle.Init.Prescaler = uwPrescalerValue;
  TimHandle.Init.ClockDivision = 0;
  TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
  TimHandle.Init.Period = mperiod; 
  if(HAL_TIM_Base_Init(&TimHandle) != HAL_OK)
  {
    
    _Error_Handler(__FILE__, __LINE__);
    /* Start the TIM time Base generation in interrupt mode */
  }
  
   sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&TimHandle, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
  
   sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&TimHandle, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
  
  HAL_TIM_Base_Start_IT(&TimHandle);

}


/**
  * @brief  This function disable the TIM2 interruption. 
  *         The time source is configured  to have X-ms  time base with a dedicated 
  *         Tick interrupt priority. 
  * @param  void
  * @retval void
  */

void ptp_interrupt_deinit(void){

	/*disable the interrupt*/
	__HAL_TIM_DISABLE_IT(&TimHandle,TIM_IT_UPDATE);
	/*disable the TIMER2 clock*/
	__TIM2_CLK_DISABLE();
	/*Interrupt_deinit*/
	HAL_NVIC_DisableIRQ(TIM2_IRQn);
}

/**
  * @brief  ptp_Suspend_interrupt.
  * @note   disabling TIM2 update interrupt.
  * @param  None
  * @retval None
  */
void ptp_interrupt_suspend(void)
{

  __HAL_TIM_DISABLE_IT(&TimHandle,TIM_IT_UPDATE);
}



/**
  * @brief  ptp_Resume_interrupt 
  * @note   Enabling TIM2 update interrupt.
  * @param  None
  * @retval None
  */
void ptp_interrupt_resume(void)
{
  /* Enable TIM2 Update interrupt */
  __HAL_TIM_ENABLE_IT(&TimHandle, TIM_IT_UPDATE);
}



/**
  * @brief  ptp_update_interrupt 
  * @note   update the parameters of the interruption.
  * @param  None
  * @retval None
  */
void ptp_update_interrupt(uint32_t period, uint32_t TickPriority)
{
	/*disable the interrupt*/
	__HAL_TIM_DISABLE_IT(&TimHandle,TIM_IT_UPDATE);
	/*disable the TIMER2 clock*/
	__TIM2_CLK_DISABLE();
	/*Interrupt_deinit*/
	HAL_NVIC_DisableIRQ(TIM2_IRQn);

	ptp_interrupt_init(period, TickPriority);

}


     


void TIM2_IRQHandler(void){  
  BSP_ADD_LED_Toggle(ADD_LED2);
  PTP_SYNC_IRQ_Handler();
  //CTRL_sync_IRQ_Handler();
  HAL_TIM_IRQHandler(&TimHandle);
}  
/***********INTERRUPT THAT CAUSE THE MEDIA SYNC UPDATE PARAMETERS*****************/

void PTP_NEW_SYNC_RESULT_SW_IRQHandler(){
HAL_GPIO_EXTI_IRQHandler(PTP_NEW_SYNC_RESULT_SW_IRQ_PIN);
UPDATE_SYNC_IRQ();
}

/**
  * @brief  ptp_to_ctrl_init 
  * @note   update the parameters of the interruption.
  * @param  None
  * @retval None
  */

void ptp_to_ctrl_init(void){
    HAL_NVIC_SetPriority((IRQn_Type)PTP_NEW_SYNC_RESULT_SW_IRQn, PTP_NEW_SYNC_RESULT_SW_IRQ_PRIORITY, 0 );
    HAL_NVIC_EnableIRQ((IRQn_Type)PTP_NEW_SYNC_RESULT_SW_IRQn);
}


/******************************************************************************/



void _Error_Handler(char * file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler_Debug */ 
}
