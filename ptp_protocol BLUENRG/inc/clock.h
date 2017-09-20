/*Clock.h*/

#ifndef _CLOCK_H_
#define _CLOCK_H_

#include "ble_clock.h"
#include "gp_timer.h"

#define TEST_PERIOD 200
#define TEST_PTP_1 0

void set_ticks(tClockTime Ctime);
void ajust_clock(int32_t Ctime);
void set_clock(tClockTime Ctime, tClockTime Cseconds);
void clock_reset(void);
void clock_wait(uint32_t i);
tClockTime sclock_time(void);
tClockTime clock_time(void);
void clock_Init(void);
void update_clock(void);
void stop_clock(void);
void resume_clock(void);
#if TEST_PTP_1
void enable_test_fun();
#endif

#endif/*_CLOCK_H_*/