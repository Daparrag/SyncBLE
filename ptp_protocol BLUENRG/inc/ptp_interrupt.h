#ifndef PTP_INTERRUPT_H
#define PTP_INTERRUPT_H

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_tim_ex.h"
#include "ptp_core.h"
#include "clock.h"
#include "debug.h"



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


//void TIM2_IRQHandler(void);
#endif /*PTP_INTERRUPT_H*/
