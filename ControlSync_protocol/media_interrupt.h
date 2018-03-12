#ifndef MEDIA_SERVICE_INTERRUPT_H
#define MEDIA_SERVICE_INTERRUPT_H


#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_tim_ex.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_tim_ex.h"
#include "clock.h"
#include "stm32f4xx_nucleo_add_led.h"
#include "debug.h"

#include "media_service.h"


void MDA_interrupt_init(uint32_t period, uint32_t TickPriority);
void MDA_update_interrupt(uint32_t period, uint32_t TickPriority);
void MDA_interrupt_deinit(void);
void MDA_interrupt_suspend(void);
void MDA_interrupt_resume(void);

#endif /**/
	