/*ptp_interupt generation*/



#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_tim_ex.h"
#include "clock.h"
#include "ptp_time_int_test.h"
#include "debug.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef        TimHandle;
/* Private function prototypes -----------------------------------------------*/
void TIM2_IRQHandler(void);
static void _Error_Handler();
/* Private functions ---------------------------------------------------------*/

#ifdef DEBUG
#include "debug.h"
#endif


static void ptp_test_print_clock(tClockTime tm)
{
#if 0
	 uint_fast32_t days;
	 uint_fast16_t stime;
	 uint_fast8_t hours;
	 uint_fast8_t minutes;
	 uint_fast8_t seconds;
	 uint_fast8_t mseconds;

days  = (uint_fast16_t)(tm/(24000UL*3600UL));
tm -= ((uint32_t)days * (24000UL*3600UL));
/*tm now contains the number of hours in the last day*/
hours = (uint_fast16_t)(tm / 3600000UL);
stime = tm - ((uint32_t)hours * 3600000UL);
/*stime now contain the number of msseconds in the last hour*/
minutes  = (stime / 60000U);
seconds  = (stime - (minutes * 60000U))/1000UL;
/*seconds now contain the number of seconds in the last minute*/
mseconds = stime - ((uint32_t)seconds * 1000UL);
#endif
	//PRINTDEBUG("Current Clock: %d:%d:%d:%d.%d \n", days, hours,minutes,seconds,mseconds);
        PRINTDEBUG ("TICKS: %d \n", tm);

}



/**
  * @brief  This function configures the TIM2 as a time base source. 
  *         The time source is configured  to have 1ms time base with a dedicated 
  *         Tick interrupt priority. 
  * @param  TickPriority: Tick interrupt priority.
  * @retval HAL status
  */
HAL_StatusTypeDef ptp_interrupt_test_init(uint32_t TickPriority)
{

  RCC_ClkInitTypeDef    clkconfig;
  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  uint32_t              uwTimclock, uwAPB1Prescaler = 0U;
  uint32_t              uwPrescalerValue = 0U;
  uint32_t              pFLatency;
  
    /*Configure the TIM2 IRQ priority */
  HAL_NVIC_SetPriority(TIM2_IRQn, TickPriority ,0U);
  
  /* Enable the TIM2 global Interrupt */
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
  
  /* Enable TIM2 clock */
 	__TIM2_CLK_ENABLE();

  /* Get clock configuration */
  HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
  
  /* Get APB1 Pre-scaler */
  uwAPB1Prescaler = clkconfig.APB1CLKDivider;
  
  /* Compute TIM2 clock */
  if (uwAPB1Prescaler == RCC_HCLK_DIV1) 
  {
    uwTimclock = HAL_RCC_GetPCLK1Freq();
  }
  else
  {
    uwTimclock = 2*HAL_RCC_GetPCLK1Freq();
  }
  
  /* Compute the prescaler value to have TIM2 counter clock equal to 1MHz */
  uwPrescalerValue = (uint32_t) ((uwTimclock / 12800000U) - 1U);
  
  /* Initialize TIM2 */
  TimHandle.Instance = TIM2;
  
  /* Initialize TIMx peripheral as follow:
  + Period = [(TIM2CLK/1000) - 1]. to have a (1/1000) s time base.
  + Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
  + ClockDivision = 0
  + Counter direction = Up
  */
  TimHandle.Init.Period = (37000000U / 1000U) - 1U;
  TimHandle.Init.Prescaler = uwPrescalerValue;
  TimHandle.Init.ClockDivision = 0;
  TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
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


  return HAL_TIM_Base_Start_IT(&TimHandle);
  //return HAL_OK;
  /* Return function status */
}

/**
  * @brief  ptp_Suspend_interrupt.
  * @note   disabling TIM2 update interrupt.
  * @param  None
  * @retval None
  */
void ptp_Suspend_test_interrupt(void)
{

	__HAL_TIM_DISABLE_IT(&TimHandle,TIM_IT_UPDATE);
}

/**
  * @brief  ptp_Resume_test_interrupt 
  * @note   Enabling TIM2 update interrupt.
  * @param  None
  * @retval None
  */
void ptp_Resume_test_interrupt(void)
{
  /* Enable TIM6 Update interrupt */
  __HAL_TIM_ENABLE_IT(&TimHandle, TIM_IT_UPDATE);
}


/**
  * @brief  ptp_Resume_test_interrupt 
  * @note   Enabling TIM2 update interrupt.
  * @param  None
  * @retval None
  */
void TIM2_IRQHandler(void){
	/*get and print the current clock*/
#ifdef DEBUG	
	tClockTime current_time = clock_time();
	ptp_test_print_clock(current_time);
#endif	
  BSP_ADD_LED_Toggle(ADD_LED2);
  HAL_TIM_IRQHandler(&TimHandle);
}

void _Error_Handler(char * file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler_Debug */ 
}
