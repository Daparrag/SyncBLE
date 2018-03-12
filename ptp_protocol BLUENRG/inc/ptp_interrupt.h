#ifndef PTP_INTERRUPT_H
#define PTP_INTERRUPT_H

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_tim_ex.h"
#include "media_service.h"
#include "ptp_core.h"
#include "clock.h"
#include "debug.h"
#include "media_sync_server.h"



typedef enum{
  UNITIALIZED,
  WAIT_SEC_TIME,
  COMPLETED
}disc_status;

struct Dscover_Cinterval{
  tClockTime time1;
  tClockTime time2;
  disc_status status;
};


/** @defgroup STM32_Connection interval_interupt_Constants 
 * @{
 */

// BNRG_CONN_INTERV_IRQ_PIN: PB.9
#define BNRG_CONN_INTERV_IRQ_PIN                GPIO_PIN_9
#define BNRG_CONN_INTERV_IRQ_MODE               GPIO_MODE_IT_RISING//_FALLING
#define BNRG_CONN_INTERV_IRQ_PULL               GPIO_PULLDOWN
#define BNRG_CONN_INTERV_IRQ_SPEED              GPIO_SPEED_HIGH
#define BNRG_CONN_INTERV_IRQ_ALTERNATE          0
#define BNRG_CONN_INTERV_IRQ_PORT               GPIOB
#define BNRG_CONN_INTERV_IRQ_CLK_ENABLE()       __GPIOB_CLK_ENABLE()

#define BNRG_CONN_INTERV_EXTI_IRQn              EXTI9_5_IRQn
#define BNRG_CONN_INTERV_EXTI_IRQHandler        EXTI9_5_IRQHandler
#define BNRG_CONN_INTERV_EXTI_PIN               BNRG_CONN_INTERV_IRQ_PIN
#define BNRG_CONN_INTERV_EXTI_PORT              BNRG_CONN_INTERV_IRQ_PORT
#define BNRG_CONN_INTERV_EXTI_PRIORITY          0x07

#define BNRG_CONN_INTERVAL_MASK_TIME                         (5) /*Mask time for connection interval in ms*/

/***********Software Interruptions to report the PTP synchonization Results****************/
#if defined(STM32F401xE) || defined (STM32F411xE) || defined (STM32F446xx) || defined (STM32L476xx)
#define PTP_NEW_SYNC_RESULT_SW_IRQn             EXTI1_IRQn
#define PTP_NEW_SYNC_RESULT_SW_IRQHandler       EXTI1_IRQHandler
#define PTP_NEW_SYNC_RESULT_SW_IRQ_PIN          GPIO_PIN_1
#define PTP_NEW_SYNC_RESULT_SW_IRQ_PRIORITY     0x01
#endif



/**********FUNCTIONS********/
void ptp_interrupt_init(uint32_t period, uint32_t TickPriority);
void ptp_update_interrupt(uint32_t period, uint32_t TickPriority);
void ptp_interrupt_deinit(void);
void ptp_interrupt_suspend(void);
void ptp_interrupt_resume(void);
void BlueNRG_ConnInterval_Init(uint32_t ConnInterval_ms);
void BlueNRG_ConnInt_Tick(void);
void BlueNRG_ConnInterval_IRQ_Callback(void);
void BlueNRG_ConnInterval_IRQ_enable(void);
void PTP_NEW_SYNC_RESULT_SW_IRQHandler();


/********PTP-to-CTRL-func********************/

/**
  * @brief  ptp_to_ctrl_init 
  * @note   update the parameters of the interruption.
  * @param  None
  * @retval None
  */

void ptp_to_ctrl_init(void);


//void TIM2_IRQHandler(void);
#endif /*PTP_INTERRUPT_H*/
