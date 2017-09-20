/**
  ******************************************************************************
  * @file    stm32f4xx_nucleo_led.c
  * @author  CL
  * @version V1.0.0
  * @date    04-July-2014
  * @brief    This file contains definitions for additiona leds
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2014 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 
  
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_nucleo_add_led.h"


/* Exported variables ---------------------------------------------------------*/
    
/** @addtogroup BSP
  * @{
  */ 

/** @addtogroup STM32F4XX_NUCLEO
  * @{
  */   
    
/** @addtogroup stm32f4xx_NUCLEO_LOW_LEVEL 
  * @brief This file contains definitions for SPI communication on
  *        stm32f4xx-Nucleo Kit from STMicroelectronics for
  *        BLE BlueNRG shield (reference X-NUCLEO-IDB04A1)
  * @{
  */ 

/** @defgroup stm32f4xx_NUCLEO_LOW_LEVEL_Private_TypesDefinitions 
  * @{
  */ 
/**
  * @}
  */ 


/** @defgroup stm32f4xx_NUCLEO_LOW_LEVEL_Private_Defines 
  * @{
  */ 

/**
  * @brief STM32F4XX NUCLEO BSP Driver version number V1.0.0
  */

/**
  * @brief LINK SD Card
  */

/**
  * @}
  */ 

/** @defgroup stm32f4xx_NUCLEO_LOW_LEVEL_Private_Macros
  * @{
  */ 
/**
  * @}
  */ 

/** @defgroup stm32f4xx_NUCLEO_LOW_LEVEL_Private_Variables
  * @{
  */
GPIO_TypeDef* ADD_LED_GPIO_PORT[ADD_LEDn] = {ADD_LED1_GPIO_PORT,ADD_LED2_GPIO_PORT,ADD_LED3_GPIO_PORT,ADD_LED4_GPIO_PORT,ADD_LED5_GPIO_PORT,ADD_LED6_GPIO_PORT,ADD_LED7_GPIO_PORT,ADD_LED8_GPIO_PORT};

const uint16_t ADD_LED_GPIO_PIN[ADD_LEDn] = {ADD_LED1_PIN,ADD_LED2_PIN,ADD_LED3_PIN,ADD_LED4_PIN,ADD_LED5_PIN,ADD_LED6_PIN,ADD_LED7_PIN,ADD_LED8_PIN};
  
/**
  * @}
  */ 

/** @defgroup stm32f4xx_NUCLEO_LOW_LEVEL_Private_FunctionPrototypes
  * @{
  */
/**
  * @}
  */ 

/** @defgroup stm32f4xx_NUCLEO_LOW_LEVEL_Private_Functions
  * @{
  */ 

/**
  * @brief  Configures LED GPIO.
  * @param  Led: Specifies the Led to be configured. 
  *   This parameter can be one of following parameters:
  *     @arg LED2
  * @retval None
  */
void BSP_ADD_LED_Init(Add_Led_TypeDef Led)
{
  GPIO_InitTypeDef  GPIO_InitStruct;
  
  /* Enable the GPIO_LED Clock */
  ADD_LEDx_GPIO_CLK_ENABLE(Led);
  //ADD_LED1_GPIO_CLK_ENABLE();
  
  /* Configure the GPIO_LED pin */
  GPIO_InitStruct.Pin = ADD_LED_GPIO_PIN[Led];
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
  
  HAL_GPIO_Init(ADD_LED_GPIO_PORT[Led], &GPIO_InitStruct);
  
  HAL_GPIO_WritePin(ADD_LED_GPIO_PORT[Led], ADD_LED_GPIO_PIN[Led], GPIO_PIN_SET); 
}

/**
  * @brief  Turns selected LED On.
  * @param  Led: Specifies the Led to be set on. 
  *   This parameter can be one of following parameters:
  *     @arg LED2
  * @retval None
  */
void BSP_ADD_LED_On(Add_Led_TypeDef Led)
{
  HAL_GPIO_WritePin(ADD_LED_GPIO_PORT[Led], ADD_LED_GPIO_PIN[Led], GPIO_PIN_SET); 
}

/**
  * @brief  Turns selected LED Off.
  * @param  Led: Specifies the Led to be set off. 
  *   This parameter can be one of following parameters:
  *     @arg LED2
  * @retval None
  */
void BSP_ADD_LED_Off(Add_Led_TypeDef Led)
{
  HAL_GPIO_WritePin(ADD_LED_GPIO_PORT[Led], ADD_LED_GPIO_PIN[Led], GPIO_PIN_RESET); 
}

/**
  * @brief  Toggles the selected LED.
  * @param  Led: Specifies the Led to be toggled. 
  *   This parameter can be one of following parameters:
  *     @arg LED2  
  * @retval None
  */
void BSP_ADD_LED_Toggle(Add_Led_TypeDef Led)
{
  HAL_GPIO_TogglePin(ADD_LED_GPIO_PORT[Led], ADD_LED_GPIO_PIN[Led]);
}

/**
  * @brief  Turns ALL DIR LED On.
  * @param  None
  * @retval None
  */
void BSP_ADD_LED_ALL_On(void){
  BSP_ADD_LED_On(ADD_LED1);  
  BSP_ADD_LED_On(ADD_LED2);  
  BSP_ADD_LED_On(ADD_LED3);  
  BSP_ADD_LED_On(ADD_LED4);  
  BSP_ADD_LED_On(ADD_LED5);  
  BSP_ADD_LED_On(ADD_LED6);  
  BSP_ADD_LED_On(ADD_LED7);  
  BSP_ADD_LED_On(ADD_LED8);  
  
}

/**
  * @brief  Turns ALL DIR LED Off.
  * @param  None
  * @retval None
  */
void BSP_ADD_LED_ALL_Off(void){
  BSP_ADD_LED_Off(ADD_LED1);  
  BSP_ADD_LED_Off(ADD_LED2);  
  BSP_ADD_LED_Off(ADD_LED3);  
  BSP_ADD_LED_Off(ADD_LED4);  
  BSP_ADD_LED_Off(ADD_LED5);  
  BSP_ADD_LED_Off(ADD_LED6);  
  BSP_ADD_LED_Off(ADD_LED7);  
  BSP_ADD_LED_Off(ADD_LED8);    
}


/**
  * @brief  Toggles ALL DIR LED.
  * @param  None
  * @retval None
  */

void BSP_ADD_LED_ALL_Toggle(void){
  BSP_ADD_LED_Toggle(ADD_LED1);  
  BSP_ADD_LED_Toggle(ADD_LED2);  
  BSP_ADD_LED_Toggle(ADD_LED3);  
  BSP_ADD_LED_Toggle(ADD_LED4);  
  BSP_ADD_LED_Toggle(ADD_LED5);  
  BSP_ADD_LED_Toggle(ADD_LED6);  
  BSP_ADD_LED_Toggle(ADD_LED7);  
  BSP_ADD_LED_Toggle(ADD_LED8);    
}

/**
  * @brief  Blinks ALL DIR LED.
* @param  Ntimes: number of blinks.
* @param  msdelay: delay between ON and OFF.
  * @retval None
  */
void BSP_ADD_LED_ALL_Blink(uint8_t NTimes, uint8_t msdelay){
  uint8_t i=0;
  
  for(i=0;i<NTimes;i++){  
    BSP_ADD_LED_ALL_On();
    HAL_Delay(msdelay);  
    BSP_ADD_LED_ALL_Off();  
    HAL_Delay(msdelay);   
  }
}

/*
void BSP_ADD_LED_L2R(void){
  BSP_ADD_LED_On(ADD_LED4);  
  HAL_Delay(25);  
  BSP_ADD_LED_Off(ADD_LED4);  
  HAL_Delay(10);  
  
  BSP_ADD_LED_On(ADD_LED3);  
  HAL_Delay(25);  
  BSP_ADD_LED_Off(ADD_LED3);  
  HAL_Delay(10);  
  
  BSP_ADD_LED_On(ADD_LED2);  
  HAL_Delay(25);  
  BSP_ADD_LED_Off(ADD_LED2);  
  HAL_Delay(10);  
  
  BSP_ADD_LED_On(ADD_LED1);  
  HAL_Delay(25);  
  BSP_ADD_LED_Off(ADD_LED1);  
  HAL_Delay(10);  
   
  BSP_ADD_LED_Off(ADD_LED1);  
  BSP_ADD_LED_Off(ADD_LED2);  
  BSP_ADD_LED_Off(ADD_LED3);  
  BSP_ADD_LED_Off(ADD_LED4);  
  }*/
  
 
/**
  * @}
  */ 

/**
  * @}
  */

/**
  * @}
  */    

/**
  * @}
  */ 
    
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
