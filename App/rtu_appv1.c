/***********************************************************************************/
/* Copyright (c) 2013, Wictor Lund. All rights reserved.			   */
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

#include <App/rtu.h>
#include <System/migrator.h>
#include <System/task_manager.h>

int _RTU_DATA_ state1 = 0;
int _RTU_DATA_ *state2 = NULL;

int rtu_requested = 0;

void cpRequestHook(cp_req_t req_type)
{
	printf("rtuappv1 checkpoint request\n");
	rtu_requested = 1;
}

int main()
{
	if (state2 == NULL) {
		state2 = apptask_malloc(sizeof(int));
		*state2 = 0;
	}
	printf("rtuapp_v1: got memory area @ 0x%x (pointer @ 0x%x)\n", (u_int32_t)state2, (u_int32_t)&state2);
	while (1) {
		if (rtu_requested) {
			/*
			 * Go into safe state and suspend
			 */
			rtu_requested = 0;
			printf("rtuappv1 now in safe state\n");
			TASK_IN_SAFE_STATE();
		}
		vTaskDelay(100);
		state1 += 1;
		*state2 += 2;
		printf("rtu_app v1: (%i,%i)\n", state1, *state2);
	}
}
