/*Clock.h*/

#ifndef _UCLOCK_H_
#define _UCLOCK_H_

#include "ble_clock.h"
#include "gp_timer.h"


#define MSECOND_TICKS     (1000)
#define SECOND_TICKS 	 (1000000)
#define PRECISION_POINTS    (1000)


#define CONVERT_CLOCK(uclock)	uclock.sec + uclock.msec/1000 + uclock.usec/10000


struct _uclock 
{
	volatile tClockTime usec;
	volatile tClockTime msec;
	volatile tClockTime sec;

};



tClockTime get_uclock_ticks(void);
struct _uclock  * get_uclock(void);
void dinit_uclock(void);
void init_uclock(void);
void reset_uclock(); 
void update_uclock();
void stop_uclock();
void resume_uclock();
void set_uclock(uint32_t offset);
void ajust_uclock(uint32_t offset);


#endif/*_UCLOCK_H_*/