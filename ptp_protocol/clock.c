/*CLOCK USED BY THE SYSTEM*/

/**
******************************************************************************
* File Name          : clock.h
* Author             : 
* Version            : V1.0.1
* Date               : 19-July-2012
* @brief
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
#include "ble_clock.h"
#include "cube_hal.h"
#include "clock.h"


#ifdef DEBUG
#include "debug.h"
#endif



static volatile tClockTime current_clock = 0;
static volatile unsigned long current_seconds = 0;
static unsigned int second_countdown = 1000;
static volatile uint8_t initClock = 0;
static volatile uint8_t stopClock = 0;

//RTIMER_CLOCK_LT(a, b); // This should give TRUE if 'a' is less than 'b', otherwise false.
//RTIMER_ARCH_SECOND; // The number of ticks per second. 





void update_clock(){
  if(!initClock){
    clock_Init();
    initClock=1;
  }
  if(!stopClock){
	current_clock++; /*time miliseconds tinks*/
	if(--second_countdown == 0){
		current_seconds+=1;
		 second_countdown = CLOCK_SECOND;
	}
        //PRINTF("clock setup to : %d ticks and %d seconds \n", current_clock, current_seconds);
  }       
}
/**
 * @brief  Clock_Init
 * @param  None
 * @retval None
 */
void clock_Init()
{
 current_clock = 0;
}

/**
 * @brief  Clock_Time
 * @param  None
 * @retval tClockTime
 */
tClockTime clock_time(void)
{	
 
 // PRINTDEBUG("Current Clock: %d ticks and %d seconds \n", current_clock, current_seconds);
  return current_clock;
  
}

/**
 * @brief  Clock_Wait Wait for a multiple of 1 ms.
 * @param  int i
 * @retval None
 */
 
 /**
 * @brief  SClock_Time
 * @param  None
 * @retval tClockTime in seconds
 */
 tClockTime sclock_Time(void)
{
	
  return current_seconds;
}
 
  /**
 * @brief  delay the clock
 * @param  None
 * @retval tClockTime in seconds
 */
 
void clock_wait(uint32_t i)
{
  
}

  /**
 * @brief  RESET the clock
 * @param  None
 * @retval NONE
 */

void clock_reset(void)
{
  current_clock=0;
  current_seconds=0;
   second_countdown = CLOCK_SECOND;
  
}

  /**
 * @brief  set the clock
 * @param  None
 * @retval NONE
 */
void set_clock(tClockTime Ctime, tClockTime Cseconds)
{
  current_clock=Ctime;
  current_seconds=Cseconds;
   second_countdown = CLOCK_SECOND;
  
}


void stop_clock(void){
    stopClock=1;
}

/**
 * @brief  set the ticks
 * @param  None
 * @retval NONE
 */

void set_ticks(tClockTime Ctime)
{
  current_clock=(Ctime);
}

void ajust_clock(int32_t offset){
  tClockTime ajusted = current_clock - (offset);
  set_ticks(ajusted);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

