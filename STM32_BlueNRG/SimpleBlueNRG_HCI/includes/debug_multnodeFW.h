/**
  ******************************************************************************
  * @file    debug.h 
  * @author  CL
  * @version V1.0.0
  * @date    13-June-2017
  * @brief   This file defines print functions for debug purposes.
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

#ifndef _M_NODE_DEBUG_H_
#define _M_NODE_DEBUG_H_

// DEBUG Macros
//#define DEBUG 0
#ifdef DEBUG
#include <stdio.h>
#undef  PRINTF
#define PRINTF(...) printf(__VA_ARGS__)
#undef  PRINTDEBUG
#define PRINTDEBUG(...) printf(__VA_ARGS__)
#undef  ERROR
#define ERROR(x)\
 		printf("[%s:%i] %s\n",__FILE__,__LINE__,(x))
#undef  PRINTADDRS                
#define PRINTADDRS(dest) printf("BLE: address 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X \n",\
                                ((uint8_t *)dest)[0],\
                                ((uint8_t *)dest)[1],\
                                ((uint8_t *)dest)[2],\
                                ((uint8_t *)dest)[3],\
                                ((uint8_t *)dest)[4],\
                                ((uint8_t *)dest)[5])/*used to print the device address*/

#undef PRINTUUID16
#define PRINTUUID16(uuid) printf("BLE: _UUID 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X \n",\
                                ((uint8_t *)uuid)[0],\
                                ((uint8_t *)uuid)[1],\
                                ((uint8_t *)uuid)[2],\
                                ((uint8_t *)uuid)[3],\
                                ((uint8_t *)uuid)[4],\
                                ((uint8_t *)uuid)[5],\
                                ((uint8_t *)uuid)[6],\
                                ((uint8_t *)uuid)[7],\
                                ((uint8_t *)uuid)[8],\
                                ((uint8_t *)uuid)[9],\
                                ((uint8_t *)uuid)[10],\
                                ((uint8_t *)uuid)[11],\
                                ((uint8_t *)uuid)[12],\
                                ((uint8_t *)uuid)[13],\
                                ((uint8_t *)uuid)[14],\
                                ((uint8_t *)uuid)[15]) /*used to print a uuid value*/ 		

#else
#define PRINTF(...)
#define PRINTDEBUG(...)
#define PRINTADDRS(dest)
#define PRINTUUID16(uuid)
#define ERROR(x)
#endif
/* Print the data travelling over the SPI in the .csv format for the GUI*/
//#define PRINT_CSV_FORMAT 
#ifdef PRINT_CSV_FORMAT
#include <stdio.h>
#define PRINT_CSV(...) printf(__VA_ARGS__)
#else
#define PRINT_CSV(...)
#endif


#endif /* _DEBUG_H_ */