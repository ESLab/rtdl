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
#define MAX_SENSORS 13
#define BUTTON_PRESSED 0x1

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
  else return 0;
  
}
void print_space(int in)
{
  int i;
  for(i=0 ; i<=get_chars(in) ; i++)
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
	printf("rtuACPv1 checkpoint request\n");
	rtu_requested = 1;
}

int main()
{
  	int i;
	uint32_t sensors[MAX_SENSORS];
	uint32_t chip_temp = 0;
	uint32_t voltage = 0;
	/*							sensor 1	sensor2		sensor3	 ocTemp		ocTempMax  ocTempMin*/
	uint32_t psensors_loc[] = {0x101FF000,0x101FF004,0x101FF008,0x101FF00C,0x101FF010,0x101FF014,0x101FF018,0x101FF01C,0x101FF020,0x101FF024,0x101FF028,0x101FF02C,0x101FF030};

	printf("ACP application v1 started\n");
	vTaskDelay(100);

	if (state == NULL) {
		state = apptask_malloc(sizeof(rtu_app_state));
		state->avg_temp = 0.0;
		state->avg_temp_old = 0.0;
		state->max_temp = 0.0;
		state->freertos_time = 0;
		init_smc_ddr();
	}

			
	for(i=0; i<MAX_SENSORS;i++){
	  sensors[i] = psensors_loc[i];
	}
#if 1
	  printf("\nwriting:\n");
	  for(i=0;i<MAX_SENSORS;i++)
	  {
		sensors[i] = i+10;
		vTaskDelay(10);
	  }
#endif
	while(1){
	  
	  /*Check for update requests*/
	  if (rtu_requested) {
		/*
		  * Go into safe state and suspend
		  */
		printf("rtuACPv1 now in safe state\n");
		rtu_requested = 0;
		state->freertos_time = xTaskGetTickCount();
		TASK_IN_SAFE_STATE();
	  }
	  
	  /*Read values from shared memory*/
	  chip_temp = (sensors[3] & 0xff00);
	  chip_temp >>= 8;
	  /*Store max temperature state*/
	  if(chip_temp > state->max_temp) state->max_temp = chip_temp;
	  
	  
	  printf("v1 debug: Onchip temperature is %d addr %x\n",sensors[3], psensors_loc[3]);
	  printf("v1 debug: Chip voltage is %d mV addr %x\n\n",sensors[6]*1000, psensors_loc[6]);
	  
	  /*Print out on display*/
	  printf("#-------------Display v1.0--------------#\n");
	  printf("| Onchip temperature is           %d ", chip_temp);
	  print_space(chip_temp); printf("C |\n");
	  printf("| Max Chip temperature:           %d ",(int)(state->max_temp));
	  print_space((int)(state->max_temp)); printf("C |\n");
	  printf("| Chip voltage is                 %d", voltage);
	  print_space(voltage); printf("mV |\n");

#if 1
	  /*Average temperature*/
	  state->avg_temp = DECAY * sensors[0] + (1.0 - DECAY)*state->avg_temp_old;
	  state->avg_temp_old = state->avg_temp;
	  printf("| Average onchip temperature:     %d ",(int)state->avg_temp);
	  print_space((int)state->avg_temp); printf("C |\n");
#endif 
	  printf("#---------------------------------------#\n\n");
	  
	  /*loop and check if panic button is pressed*/
	  for(i=0 ; i<10 ; i++){
#if 1
		if(sensors[12] == BUTTON_PRESSED){
		  printf("Panic button has been pressed!\nPlease reset system\n");
		  while(1)
			;
		}
#endif
		vTaskDelay(500);
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

