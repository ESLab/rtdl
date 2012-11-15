/***********************************************************************************/
/* Copyright (c) 2013, Simon Holmbacka. All rights reserved.			   */
/* Copyright (c) 2013, Åbo Akademi University. All rights reserved.		   */
/* 										   */
/* Redistribution and use in source and binary forms, with or without		   */
/* modification, are permitted provided that the following conditions are met:	   */
/*      * Redistributions of source code must retain the above copyright	   */
/*        notice, this list of conditions and the following disclaimer.		   */
/*      * Redistributions in binary form must reproduce the above copyright	   */
/*        notice, this list of conditions and the following disclaimer in the	   */
/*        documentation and/or other materials provided with the distribution.	   */
/*      * Neither the name of the Åbo Akademi University nor the		   */
/*        names of its contributors may be used to endorse or promote products	   */
/*        derived from this software without specific prior written permission.	   */
/* 										   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND */
/* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   */
/* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE	   */
/* DISCLAIMED. IN NO EVENT SHALL ÅBO AKADEMI UNIVERSITY BE LIABLE FOR ANY	   */
/* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES	   */
/* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;	   */
/* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND	   */
/* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT	   */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS   */
/* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 		   */
/***********************************************************************************/

#include <FreeRTOS.h>

#include <task.h>
#include <semphr.h>
#include <stdio.h>
#include <App/rtu_ACP.h>
#include <App/rtu.h>
#include "smc/smc_driverL.h"

#define DECAY 0.9
#define MAX_SENSORS 16
#define BUTTON_PRESSED 0x1
#define OFFSET 0
#define DEBUG 0

rtu_app_state _RTU_DATA_ *state = NULL;

int rtu_requested = 0;

int get_chars(int in)
{
  if(in>=0 && in<10)
	return 1;
  else if(in>=10 && in<100)
	return 2;
  else if(in>=100 && in<1000)
	return 3;
  else if(in > 1000)
	return 4;
  else if(in<0 && in>-10)
	return 2;
  else if(in<=-10 && in>-100)
	return 3;
  else if(in<=-100 && in>-1000)
	return 4;
  else return 0;
}

void print_space(int in)
{
  int i;
  for(i=4 ; i>get_chars(in) ; i--)
	printf(" ");
}

int get_decimal(double in)
{
  int comp = (int)in;
  if(in - comp >= 0.9)
	return 9;
  else if ((in - comp >= 0.8) && (in - comp < 0.9))
	return 8;
  else if ((in - comp >= 0.7) && (in - comp < 0.8))
	return 7;
  else if ((in - comp >= 0.6) && (in - comp < 0.7))
	return 6;  
  else if ((in - comp >= 0.5) && (in - comp < 0.6))
	return 5;
  else if ((in - comp >= 0.4) && (in - comp < 0.5))
	return 4;
  else if ((in - comp >= 0.3) && (in - comp < 0.4))
	return 3;
  else if ((in - comp >= 0.2) && (in - comp < 0.3))
	return 2;
  else if ((in - comp >= 0.1) && (in - comp < 0.2))
	return 1;
  else if ((in - comp >= 0.0) && (in - comp < 0.1))
	return 0;
  
  return 0;
}
void clear_display()
{
  int i;
  for(i=0; i<20; i++)
	printf("\n\n");
}

void cpRequestHook(cp_req_t req_type)
{
#if(DEBUG == 1)
	printf("rtuACPv1 checkpoint request\n");
#endif
	rtu_requested = 1;
}

int main()
{
  	int i;
	uint32_t sensors[MAX_SENSORS];
	double chip_temp = 0;
	double voltage = 0;
	int panic = 0;

#if (OFFSET == 0)
	uint32_t * psensors_loc[] = {
	  (uint32_t *)0x10000000,	//sensor1
	  (uint32_t *)0x10000004,	//sensor2
	  (uint32_t *)0x10000008,	//sensor3
	  (uint32_t *)0x1000000C,	//OnChip Temp
	  (uint32_t *)0x10000040,	//VccIn
	  (uint32_t *)0x10000044,	//VccAux
	  (uint32_t *)0x10000048,	//MaxTemp
	  (uint32_t *)0x1000004C,	//MaxVccIn
	  (uint32_t *)0x10000080,	//MaxVccAux
	  (uint32_t *)0x10000084,	//MinTemp
	  (uint32_t *)0x10000088,	//MinVccIn
	  (uint32_t *)0x1000008C,	//MinVccAux
	  (uint32_t *)0x100000C0,	//StatusC0
	  (uint32_t *)0x100000C4,	//StatusC1
	  (uint32_t *)0x101FF0C8,	//Inconsistence core0
	  (uint32_t *)0x101FF0CC	//Inconsistence core1
	};
#endif
#if (OFFSET == 1)
	uint32_t * psensors_loc[] = {
	  (uint32_t *)0x101FF000,
	  (uint32_t *)0x101FF004,
	  (uint32_t *)0x101FF008,
	  (uint32_t *)0x101FF00C,
	  (uint32_t *)0x101FF040,
	  (uint32_t *)0x101FF044,
	  (uint32_t *)0x101FF048,
	  (uint32_t *)0x101FF04C,
	  (uint32_t *)0x101FF080,
	  (uint32_t *)0x101FF084,
	  (uint32_t *)0x101FF088,
	  (uint32_t *)0x101FF08C,
	  (uint32_t *)0x101FF0C0,
	  (uint32_t *)0x101FF0C4,
	  (uint32_t *)0x101FF0C8,
	  (uint32_t *)0x101FF0CC
	};
#endif


#define EXT_TEMP sensors[0]
#define ONCHIP_TEMP sensors[3]
#define ONCHIP_VOLTAGE sensors[4]
#define CORE0_PANIC sensors[12]
#define CORE1_PANIC sensors[13]
#define CORE0_INCONSITENCE sensors[14]
#define CORE1_INCONSITENCE sensors[15]
	
#if(DEBUG == 1)
	printf("ACP application v1 started\n");
#endif
	vTaskDelay(100);
	init_smc_ddr();
	
	if (state == NULL) {
		state = apptask_malloc(sizeof(rtu_app_state));
		state->avg_temp = 0.0;
		state->avg_temp_old = 0.0;
		state->max_temp = -999.0;
		state->freertos_time = 0;
	}
	
#if 0
	  printf("\nwriting:\n");
	  for(i=0;i<MAX_SENSORS;i++)
	  {
		*psensors_loc[i] = i+9999;
		vTaskDelay(10);
	  }
#endif

	while(1){
	  
	  /*Check for update requests*/
	  if (rtu_requested) {
		
		/*Go into safe state and suspend*/
#if (DEBUG == 1)
		printf("rtuACPv1 now in safe state\n");
#endif
		rtu_requested = 0;
		state->freertos_time = xTaskGetTickCount();
		TASK_IN_SAFE_STATE();
	  }
	  
	  /*Read values from shared memory*/

	  for(i=0 ; i<MAX_SENSORS ; i++){
		sensors[i] = * psensors_loc[i];
#if(DEBUG == 1)
		printf("v1 debug: item %d has value %d\n",i, sensors[i]);
#endif
	  }

	  chip_temp = ((double)ONCHIP_TEMP * 503.975 / 1024.0) - 273.15;
	  voltage = ((double)ONCHIP_VOLTAGE*1000.0 / 1024.0) * 3.0;			//for milli volts
	  
	  /*Store max temperature state*/
	  if(chip_temp > state->max_temp) state->max_temp = chip_temp;



	  /*Print out on display*/
	  printf("#--------------Display v1.0--------------#\n");
	  printf("| Onchip temperature is           %d ", (int)chip_temp);
	  print_space(chip_temp); printf("C |\n");
	  printf("| Max Chip temperature:           %d ",(int)(state->max_temp));
	  print_space((int)(state->max_temp)); printf("C |\n");

	  /*Average temperature*/
	  state->avg_temp = DECAY * chip_temp + (1.0 - DECAY)*state->avg_temp_old;
	  state->avg_temp_old = state->avg_temp;
	  printf("| Average onchip temperature:     %d ",(int)state->avg_temp);
	  print_space((int)state->avg_temp); printf("C |\n");
	  printf("| Internal Chip voltage is        %d", (int)voltage);
	  print_space(voltage); printf("mV |\n");
	  
	  CORE0_INCONSITENCE = * psensors_loc[14];
	  CORE1_INCONSITENCE = * psensors_loc[15];
	  if(CORE0_INCONSITENCE != 1)
		printf("| Core 0 status: OK                      |\n");
	  else
		printf("| Core 0 status: SIGNAL                  |\n"); 
	  if(CORE1_INCONSITENCE != 1)
		printf("| Core 1 status: OK                      |\n");
	  else
		printf("| Core 1 status: SIGNAL                  |\n");
	  printf("#----------------------------------------#\n\n");
	  if(panic == 1)
		printf("**Panic button has been pressed!**\n**Please reset system**\n");
	  printf("\n\n\n\n\n");
	  /*loop and check if panic button is pressed*/
	  for(i=0 ; i<100 ; i++){

		CORE0_PANIC = * psensors_loc[12];
		CORE1_PANIC = * psensors_loc[13];
		if((CORE0_PANIC == BUTTON_PRESSED)||(CORE1_PANIC == BUTTON_PRESSED)){
		  panic = 1;
		  printf("**Panic button has been pressed!****\n****Please reset system**\n");
		  while(1)
			;
		}

		vTaskDelay(25);
	  }
	  
	}
  return 0;
}

void vApplicationMallocFailedHook( void )
{
#if defined(VEXPRESS_VM) || defined(VEXPRESS_NOVM)
	__asm volatile (" smc #0 ");
#endif
}

