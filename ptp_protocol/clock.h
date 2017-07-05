/*Clock.h*/

#ifndef _BLE_CLOCK_H_
#define _BLE_CLOCK_H_

#include "ble_clock.h"
#include "gp_timer.h"

void set_ticks(tClockTime Ctime);
void set_clock(tClockTime Ctime, tClockTime Cseconds);
void Clock_reset(void);
void Clock_Wait(uint32_t i);
tClockTime SClock_Time(void);
tClockTime Clock_Time(void);
void clock_Init(void);
void update_clock(void);
#endif/*_BLE_CLOCK_H_*/