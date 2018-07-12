#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_tim_ex.h"
#include "clock_interface.h"
#include "ptp_interrupt.h"
#include "stm32f4xx_nucleo_add_led.h"
#ifdef DEBUG
#include "debug.h"
#endif
#include "conn_inval_interrupt.h"
//#include "bluevoice_sync_service.h"


#define TEST_CONN_INTVAL 0

static void BlueNRG_Discovery_cinval(void);
static void BlueNRG_Discovery_cinval_next_state(disc_status status);
static struct Dscover_Cinterval dcvr_intv = {0,0,UNITIALIZED};
static cinval_interrupt_status Conn_intval_state = CONN_INTVAL_UNSTARTED;

/******************************************************************************/
/*

   ___                            _   _                _____       _                       _ 
  / __\___  _ __  _ __   ___  ___| |_(_) ___  _ __     \_   \_ __ | |_ ___ _ ____   ____ _| |
 / /  / _ \| '_ \| '_ \ / _ \/ __| __| |/ _ \| '_ \     / /\/ '_ \| __/ _ \ '__\ \ / / _` | |
/ /__| (_) | | | | | | |  __/ (__| |_| | (_) | | | | /\/ /_ | | | | ||  __/ |   \ V / (_| | |
\____/\___/|_| |_|_| |_|\___|\___|\__|_|\___/|_| |_| \____/ |_| |_|\__\___|_|    \_/ \__,_|_|


*/



volatile uint8_t BlueNRG_ConnIntervalValid = 0;
volatile uint8_t BlueNRG_ConnInterval_count = 0;
volatile uint8_t pending_sync_transmission =0;
volatile uint8_t connection_id=0;


/**
  * @brief  This function configures the connection interval pint interruption. 
  *         The time source is configured  to have X-ms  time base with a dedicated 
  *         Tick interrupt priority. 
  * @param  uint16_t period: Tick interrupt period (ms).
  * @retval void
  */

void BlueNRG_ConnInterval_Init(uint32_t ConnInterval_ms) 
{
  GPIO_InitTypeDef GPIO_InitStruct;

  /* TEST 1IRQ -- INPUT */
  GPIO_InitStruct.Pin = BNRG_CONN_INTERV_IRQ_PIN;
  GPIO_InitStruct.Mode = BNRG_CONN_INTERV_IRQ_MODE;
  GPIO_InitStruct.Pull = BNRG_CONN_INTERV_IRQ_PULL;
  GPIO_InitStruct.Speed = BNRG_CONN_INTERV_IRQ_SPEED;
  GPIO_InitStruct.Alternate = BNRG_CONN_INTERV_IRQ_ALTERNATE;
  HAL_GPIO_Init(BNRG_CONN_INTERV_IRQ_PORT, &GPIO_InitStruct);
  Conn_intval_state = CONN_INTVAL_DISABLE;
}




/**
 * @brief  Timer callback to handle CONN_INTERV_IRQ
 * @param  None
 * @retval None
 */
void BlueNRG_ConnInt_Tick(void)
{
  if(BlueNRG_ConnIntervalValid)
  {
    BlueNRG_ConnInterval_count++;
    if(BlueNRG_ConnInterval_count==BNRG_CONN_INTERVAL_MASK_TIME)
    {
      __HAL_GPIO_EXTI_CLEAR_IT(BNRG_CONN_INTERV_IRQ_PIN);
      HAL_NVIC_ClearPendingIRQ(BNRG_CONN_INTERV_EXTI_IRQn);
      HAL_NVIC_EnableIRQ(BNRG_CONN_INTERV_EXTI_IRQn);
      BlueNRG_ConnInterval_count=0;
      BlueNRG_ConnIntervalValid=0;
    }
  }
  
}


void BlueNRG_ConnInterval_IRQ_Callback(void)
{
//#if defined (CINTERVAL_SERVER)  
 // BSP_ADD_LED_On(ADD_LED8);
 //BSP_ADD_LED_Off(ADD_LED8);
//#endif  
  BlueNRG_ConnIntervalValid = 1;
  HAL_NVIC_DisableIRQ(BNRG_CONN_INTERV_EXTI_IRQn);
  __HAL_GPIO_EXTI_CLEAR_IT(BNRG_CONN_INTERV_IRQ_PIN);
  HAL_NVIC_ClearPendingIRQ(BNRG_CONN_INTERV_EXTI_IRQn);
  if(dcvr_intv.status==COMPLETED){
    
 #if (TEST_CONN_INTVAL)   
    if(connection_id==0){
    BSP_ADD_LED_On(ADD_LED3);
    
    }else if(connection_id==1)
    {
        
        BSP_ADD_LED_On(ADD_LED8);
    }
//BLUEVOICE_Cinterval_Process_2_Ext(connection_id);/*USE THIS only for effciency improviment otherwise use BLUEVOICE_Cinterval_Process_Ext(connection_id); */
#else   
  //BLUEVOICE_Cinterval_Process_2_Ext(connection_id);/*USE THIS only for effciency improviment otherwise use BLUEVOICE_Cinterval_Process_Ext(connection_id); */
 // BLUEVOICE_Cinterval_Process_Ext(connection_id);  
  //Media_cinterval_IRQ_Handler(connection_id);
#endif  
  
    
#if defined (CINTERVAL_SERVER)
    connection_id= !connection_id;
#elif defined (CINTERVAL_CLIENT)
     connection_id=0;
#endif     
  }else{
    BlueNRG_Discovery_cinval(); 
  }

}

void BlueNRG_ConnInterval_IRQ_enable (void)
{
  HAL_NVIC_EnableIRQ(BNRG_CONN_INTERV_EXTI_IRQn);
  HAL_NVIC_SetPriority((IRQn_Type) BNRG_CONN_INTERV_EXTI_IRQn, BNRG_CONN_INTERV_EXTI_PRIORITY, 0);          
  Conn_intval_state = CONN_INTVAL_ENABLE;
}


void BlueNRG_CoonInterval_IRQ_disable(void){
 HAL_NVIC_DisableIRQ(BNRG_CONN_INTERV_EXTI_IRQn);
 Conn_intval_state = CONN_INTVAL_DISABLE;
}


void BlueNRG_Discovery_cinval_next_state(disc_status input_status)
{
  dcvr_intv.status=input_status;
}


static void BlueNRG_Discovery_cinval(){
  
  switch (dcvr_intv.status){
    
    case UNITIALIZED:
      {
        dcvr_intv.time1 = GET_CLOCK;
        BlueNRG_Discovery_cinval_next_state(WAIT_SEC_TIME);
      }
      break;
      
    case WAIT_SEC_TIME:
      {
        dcvr_intv.time2 = GET_CLOCK;
        {
          if( ( MICRO2MILSECONDS(dcvr_intv.time2 - dcvr_intv.time1)) <= 8.5f ){
          //if( ( MICRO2MILSECONDS(dcvr_intv.time2 - dcvr_intv.time1) - ((uint32_t)EXPECTED_DELAY)) <= 8.5f ){
            connection_id=1;
              
          }else{
            connection_id=0;
            //BlueNRG_Discovery_connection_set_next_status(UNITIALIZED);    
          }
           BlueNRG_Discovery_cinval_next_state(COMPLETED); 
              
        }
      }
      break;      
    default:
    break;
  }
  
}
