#ifndef CONN_INTERVAL_INTERUPT_H
#define CONN_INTERVAL_INTERUPT_H

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_tim_ex.h"
#include "media_service.h"
#include "ptp_core.h"
#include "clock_interface.h"
#include "debug.h"
#include "RTCP_core.h"


#if  !defined(CINTERVAL_SERVER) && !defined(CINTERVAL_CLIENT)
#error "please define one of the Connection interval modes :  (CINTERVAL_SERVER: this devices is the owner of the cinterval management multiple-clientes) &(CINTERVAL_CLIENT: this is the client or the cinterval with a single connection to a cinterval server )"
#endif


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

typedef enum{
  CONN_INTVAL_UNSTARTED,
  CONN_INTVAL_DISABLE,
  CONN_INTVAL_ENABLE
}cinval_interrupt_status;

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
#define BNRG_CONN_INTERV_EXTI_PRIORITY          0x0A

#define BNRG_CONN_INTERVAL_MASK_TIME                         (5) /*Mask time for connection interval in ms*/



void BlueNRG_ConnInterval_Init(uint32_t ConnInterval_ms);
void BlueNRG_ConnInt_Tick(void);
void BlueNRG_ConnInterval_IRQ_Callback(void);
void BlueNRG_ConnInterval_IRQ_enable(void);




#endif /*CONN_INTERVAL_INTERUPT_H*/
