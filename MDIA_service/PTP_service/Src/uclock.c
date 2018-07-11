/*clock microsecond version*/



#include "ble_clock.h"
#include "cube_hal.h"
#include "uclock.h"



static volatile uint32_t count;
static volatile uint32_t mseg_count = MSECOND_TICKS;
static volatile uint32_t seg_count= SECOND_TICKS;

static volatile uint8_t initUClock = FALSE;
static volatile uint8_t stopUClock = FALSE;



struct _uclock uclock;



tClockTime get_uclock_ticks(void)
{
  return (tClockTime)count; 

}

/**
 * @brief  get_clock returns the current clock in microsenconds
 * @param  None
 * @retval NONE

 */
struct _uclock * get_uclock()
{
	return &uclock;
}




/**
 * @brief  dinitialize a micrsecond clock 
 * @param  None
 * @retval NONE
 */

void dinit_uclock()
{
        reset_uclock();
	initUClock = FALSE;

}


/**
 * @brief  initialize a micrsecond clock 
 * @param  None
 * @retval NONE
 */

void init_uclock()
{
	reset_uclock();
	initUClock = TRUE;

}


/**
 * @brief  reset a  micrsecond clock 
 * @param  None
 * @retval NONE
 */
void reset_uclock()
{
	uclock.usec = 0;
	uclock.msec = 0;
	uclock.sec = 0;


	count = 0;
	mseg_count = MSECOND_TICKS;
	seg_count= SECOND_TICKS;

}



/**
 * @brief  increment the current clock. 
 * @param  None
 * @retval NONE
 */

void update_uclock()
{


 if (initUClock != TRUE ) init_uclock();



 if (stopUClock == TRUE) return;


	count++;
 /*       uclock.usec++;
	if(--mseg_count==0)
	{
		uclock.usec = 0;
		uclock.msec++;
		mseg_count = MSECOND_TICKS;

	}

	if (--seg_count == 0)
	{
                uclock.msec =0;
                uclock.sec++;
                mseg_count = MSECOND_TICKS;
		seg_count = SECOND_TICKS;
	}	


*/	

}


/**
 * @brief  stop the current clock. 
 * @param  None
 * @retval NONE
 */

void stop_uclock()
{

	stopUClock = TRUE;
}


/**
 * @brief  resume the current clock. 
 * @param  None
 * @retval NONE
 */

void resume_uclock()
{

	stopUClock = FALSE;
}




void set_uclock(uint32_t offset)
{
	uint32_t tmp_sec;
	uint32_t tmp_msec;
	uint32_t tmp_usec;
	float tmp1;
	float tmp2;


	tmp_sec = (uint32_t) offset;
	tmp1 = ((float)tmp_sec - offset) * PRECISION_POINTS;
	tmp_msec = (uint32_t) tmp1;
	tmp2 = ((float)tmp_msec - tmp1) * PRECISION_POINTS;	
	tmp_usec = (uint32_t) (tmp2);


	stop_uclock ();

	uclock.sec = tmp_sec; 
	uclock.msec =  tmp_msec;
	uclock.usec = tmp_usec;

	resume_uclock();


}



/**
 * @brief  update the microsencond clock. 
 * @param  None
 * @retval NONE
 */

void ajust_uclock(uint32_t offset){

	uint32_t tmp_clock =  count - (offset);
	//set_uclock (tmp_clock);
        count = tmp_clock;

}