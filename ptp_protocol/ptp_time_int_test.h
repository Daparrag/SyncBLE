#ifndef PTP_TIME_INT_TEST_H
#define PTP_TIME_INT_TEST_H


#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_tim_ex.h"
#include "stm32f4xx_nucleo_add_led.h"
#include "clock.h"

HAL_StatusTypeDef ptp_interrupt_test_init(uint32_t TickPriority);
void ptp_Suspend_test_interrupt(void);
void ptp_Resume_test_interrupt(void);
void TIM2_IRQHandler(void);

#endif /*PTP_TEST_H*/