/** 
******************************************************************************
* @file    stm32f4xx_nucleo.h
* @author  MCD Application Team
* @version V1.1.0
* @date    19-June-2014
* @brief   This file contains definitions for:
*          - LEDs and push-button available on STM32F4XX-Nucleo Kit 
*            from STMicroelectronics
*          - LCD, joystick and microSD available on Adafruit 1.8" TFT LCD 
*            shield (reference ID 802)
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F4XX_NUCLEO_ADD_LED_H
#define __STM32F4XX_NUCLEO_ADD_LED_H

#ifdef __cplusplus
extern "C" {
#endif
  
  /* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "cube_hal.h"
  
  /** @addtogroup BSP
  * @{
  */
  
  /** @addtogroup STM32F4XX_NUCLEO
  * @{
  */
  
  /** @addtogroup STM32F4XX_NUCLEO_LOW_LEVEL
  * @{
  */ 
  
  /** @defgroup STM32F4XX_NUCLEO_LOW_LEVEL_Exported_Types
  * @{
  */
  typedef enum 
  {
    ADD_LED1 = 0,
    ADD_LED2 = 1,
    ADD_LED3 = 2,
    ADD_LED4 = 3,
    ADD_LED5 = 4,
    ADD_LED6 = 5,
    ADD_LED7 = 6,
    ADD_LED8 = 7    
  }Add_Led_TypeDef;
  
  
  /**
  * @}
  */ 
  
  /** @defgroup STM32F4XX_NUCLEO_LOW_LEVEL_Exported_Constants
  * @{
  */ 
  
  /** 
  * @brief Define for STM32F4XX_NUCLEO board  
  */ 
#if !defined (USE_STM32F4XX_NUCLEO)
#define USE_STM32F4XX_NUCLEO
#endif
  
  /** @addtogroup STM32F4XX_NUCLEO_LOW_LEVEL_LED
  * @{
  */
  
  
#define ADD_LEDn                                 11

#define ADD_LED11_PIN                                GPIO_PIN_0
#define ADD_LED11_GPIO_PORT                          GPIOC
#define ADD_LED11_GPIO_CLK_ENABLE()                  __GPIOC_CLK_ENABLE()
#define ADD_LED11_GPIO_CLK_DISABLE()                 __GPIOC_CLK_DISABLE()
  
#define ADD_LED10_PIN                                GPIO_PIN_5
#define ADD_LED10_GPIO_PORT                          GPIOC
#define ADD_LED10_GPIO_CLK_ENABLE()                  __GPIOC_CLK_ENABLE()
#define ADD_LED10_GPIO_CLK_DISABLE()                 __GPIOC_CLK_DISABLE()
  
#define ADD_LED9_PIN                                GPIO_PIN_1
#define ADD_LED9_GPIO_PORT                          GPIOC
#define ADD_LED9_GPIO_CLK_ENABLE()                  __GPIOC_CLK_ENABLE()
#define ADD_LED9_GPIO_CLK_DISABLE()                 __GPIOC_CLK_DISABLE()  
  
#define ADD_LED8_PIN                                GPIO_PIN_12
#define ADD_LED8_GPIO_PORT                          GPIOC
#define ADD_LED8_GPIO_CLK_ENABLE()                  __GPIOC_CLK_ENABLE()
#define ADD_LED8_GPIO_CLK_DISABLE()                 __GPIOC_CLK_DISABLE()
  
#define ADD_LED7_PIN                                GPIO_PIN_9
#define ADD_LED7_GPIO_PORT                          GPIOC
#define ADD_LED7_GPIO_CLK_ENABLE()                  __GPIOC_CLK_ENABLE()
#define ADD_LED7_GPIO_CLK_DISABLE()                 __GPIOC_CLK_DISABLE()
  
#define ADD_LED6_PIN                                GPIO_PIN_8
#define ADD_LED6_GPIO_PORT                          GPIOC
#define ADD_LED6_GPIO_CLK_ENABLE()                  __GPIOC_CLK_ENABLE()
#define ADD_LED6_GPIO_CLK_DISABLE()                 __GPIOC_CLK_DISABLE()
  
#define ADD_LED5_PIN                                GPIO_PIN_2
#define ADD_LED5_GPIO_PORT                          GPIOC
#define ADD_LED5_GPIO_CLK_ENABLE()                  __GPIOC_CLK_ENABLE()
#define ADD_LED5_GPIO_CLK_DISABLE()                 __GPIOC_CLK_DISABLE()
  
#define ADD_LED4_PIN                                GPIO_PIN_3
#define ADD_LED4_GPIO_PORT                          GPIOC
#define ADD_LED4_GPIO_CLK_ENABLE()                  __GPIOC_CLK_ENABLE()
#define ADD_LED4_GPIO_CLK_DISABLE()                 __GPIOC_CLK_DISABLE()
  
#define ADD_LED3_PIN                                GPIO_PIN_4
#define ADD_LED3_GPIO_PORT                          GPIOA
#define ADD_LED3_GPIO_CLK_ENABLE()                  __GPIOA_CLK_ENABLE()
#define ADD_LED3_GPIO_CLK_DISABLE()                 __GPIOA_CLK_DISABLE()
  
#define ADD_LED2_PIN                                GPIO_PIN_0
#define ADD_LED2_GPIO_PORT                          GPIOB
#define ADD_LED2_GPIO_CLK_ENABLE()                  __GPIOB_CLK_ENABLE()
#define ADD_LED2_GPIO_CLK_DISABLE()                 __GPIOB_CLK_DISABLE()
  
#define ADD_LED1_PIN                                GPIO_PIN_10
#define ADD_LED1_GPIO_PORT                          GPIOC
#define ADD_LED1_GPIO_CLK_ENABLE()                  __GPIOC_CLK_ENABLE()
#define ADD_LED1_GPIO_CLK_DISABLE()                 __GPIOC_CLK_DISABLE()  
  

#define ADD_LEDx_GPIO_CLK_ENABLE(__INDEX__)    do{ if((__INDEX__) == 0) ADD_LED1_GPIO_CLK_ENABLE(); \
                                                  if((__INDEX__) == 1) ADD_LED2_GPIO_CLK_ENABLE(); \
                                                    if((__INDEX__) == 2) ADD_LED3_GPIO_CLK_ENABLE(); \
                                                      if((__INDEX__) == 3) ADD_LED4_GPIO_CLK_ENABLE(); \
                                                        if((__INDEX__) == 4) ADD_LED5_GPIO_CLK_ENABLE(); \
                                                          if((__INDEX__) == 5) ADD_LED6_GPIO_CLK_ENABLE(); \
                                                            if((__INDEX__) == 6) ADD_LED7_GPIO_CLK_ENABLE(); \
                                                              if((__INDEX__) == 7) ADD_LED8_GPIO_CLK_ENABLE(); \
                                                                if((__INDEX__) == 8) ADD_LED9_GPIO_CLK_ENABLE(); \
                                                                  if((__INDEX__) == 9) ADD_LED10_GPIO_CLK_ENABLE(); \
                                                                    if((__INDEX__) == 10) ADD_LED11_GPIO_CLK_ENABLE(); \
}while(0)  
  
#define ADD_LEDx_GPIO_CLK_DISABLE(__INDEX__)    do{ if((__INDEX__) == 0) ADD_LED1_GPIO_CLK_DISABLE(); \
                                                  if((__INDEX__) == 1) ADD_LED2_GPIO_CLK_DISABLE(); \
                                                    if((__INDEX__) == 2) ADD_LED3_GPIO_CLK_DISABLE(); \
                                                      if((__INDEX__) == 3) ADD_LED4_GPIO_CLK_DISABLE(); \
                                                        if((__INDEX__) == 4) ADD_LED5_GPIO_CLK_DISABLE(); \
                                                          if((__INDEX__) == 5) ADD_LED6_GPIO_CLK_DISABLE(); \
                                                            if((__INDEX__) == 6) ADD_LED7_GPIO_CLK_DISABLE(); \
                                                              if((__INDEX__) == 7) ADD_LED8_GPIO_CLK_DISABLE(); \
                                                                if((__INDEX__) == 8) ADD_LED9_GPIO_CLK_DISABLE(); \
                                                                  if((__INDEX__) == 9) ADD_LED10_GPIO_CLK_DISABLE(); \
                                                                    if((__INDEX__) == 10) ADD_LED11_GPIO_CLK_DISABLE(); \
}while(0)  

/**
* @}
*/ 


/** @defgroup STM32F4XX_NUCLEO_LOW_LEVEL_Exported_Macros
* @{
*/  
/**
* @}
*/ 

/** @defgroup STM32F4XX_NUCLEO_LOW_LEVEL_Exported_Functions
* @{
*/
void             BSP_ADD_LED_Init(Add_Led_TypeDef Led);
void             BSP_ADD_LED_On(Add_Led_TypeDef Led);
void             BSP_ADD_LED_Off(Add_Led_TypeDef Led);
void             BSP_ADD_LED_Toggle(Add_Led_TypeDef Led);
void             BSP_ADD_LED_ALL_On(void);
void             BSP_ADD_LED_ALL_Off(void);
void             BSP_ADD_LED_ALL_Toggle(void);
void             BSP_ADD_LED_ALL_Blink(uint8_t NTimes, uint8_t msdelay);

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

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4XX_NUCLEO_ADD_LED_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
