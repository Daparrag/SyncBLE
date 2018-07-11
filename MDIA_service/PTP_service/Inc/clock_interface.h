#ifndef _CLOCK_INTERFACE_H_
#define _CLOCK_INTERFACE_H_

/*ptp_clock_interface*/
/*this interface allows to the user operate the PTP clock ether in miliseconds or microseconds without affecting the protocol 
 *configuration
*/




#if !defined (MICROSECONDS_CLOCK) && !defined(MILISECODS_CLOCK)
#error "your are not setting the acccuracy of the clock. please define MICROSECONDS_CLOCK or MILISECODS_CLOCK in a configuration file"
#endif


#if defined (MICROSECONDS_CLOCK)
#include "uclock.h" 	
#elif defined (MILISECODS_CLOCK)
#include "msclock.h" 	
#endif	





#if defined (MICROSECONDS_CLOCK)

#define GET_CLOCK get_uclock_ticks ()
#define AJUST_CLOCK(value) ajust_uclock(value)
#define CLOCK_RESET reset_uclock()
#define CLOCK_INIT	init_uclock()
#define UPDATE_CLOCK update_uclock()

#define CLOCK_PERIOD	((1000000 / 10000) - 1) /*0.1ms*/
#define MICRO2MILSECONDS(value) ((value) / 10) 


#elif defined  (MILISECODS_CLOCK)


#define GET_CLOCK clock_time ()
#define AJUST_CLOCK(value) ajust_clock(value)
#define CLOCK_RESET clock_reset()
#define CLOCK_INIT	clock_Init()
#define UPDATE_CLOCK update_clock()

#define CLOCK_PERIOD  ((1000000 / 1000) - 1) /*1.ms*/
#define MICRO2MILSECONDS(value) ((value) / 1) /*we are eirking already in ms*/ 
#endif
#endif/* _CLOCK_INTERFACE_H_*/


