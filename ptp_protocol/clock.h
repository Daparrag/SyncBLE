/*Clock.h*/

#ifndef _CLOCK_H_
#define _CLOCK_H_

#include "ble_clock.h"
#include "gp_timer.h"

void set_ticks(tClockTime Ctime);
void set_clock(tClockTime Ctime, tClockTime Cseconds);
void clock_reset(void);
void clock_wait(uint32_t i);
tClockTime sclock_time(void);
tClockTime clock_time(void);
void clock_Init(void);
void update_clock(void);
#endif/*_CLOCK_H_*/