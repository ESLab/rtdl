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

#define VX_MAIN VX_MAIN
#define SYSTEM_MODULE VX_MAIN

#include <FreeRTOS.h>

#include <task.h>
#include <queue.h>

#include <System/types.h>
#include <System/system.h>
#include <System/arch/vexpress_vm/memory_layout.h>
#include <System/arch/vexpress_vm/binary_register.h>
#include <System/umm/umm_malloc.h>

static void setup_heap()
{
	xMemoryInformationType	*mit	   = MIS_START_ADDRESS;
	void			*heap	   = mit[portCORE_ID()].phys_heap_begin;
	size_t			 heap_size = mit[portCORE_ID()].phys_heap_size - 0x10000;

	umm_init(heap, heap_size);
}

portTASK_FUNCTION(native_task, arg)
{
	int i = 0;
	while (1) {
		INFO_MSG("Hmm... (%i)\n", ++i);
		vTaskDelay(200);
	}
}

portTASK_FUNCTION(trivial_migrator, arg)
{
	xTaskHandle h = arg;

	INFO_MSG("%s: Task handle = 0x%x\n", __func__, (npi_t)h);

	INFO_MSG("%s: Trivial migrator start.\n", __func__);
	while (1) {
		vTaskDelay(1000);
		if (xTaskDetach(h) != pdPASS) {
			ERROR_MSG("%s: failed to detach native task.\n", __func__);
			goto error;
		} else {
			INFO_MSG("%s: successfully detached native task.\n", __func__);
		}

		vTaskDelay(1000);
		if (xTaskAttach(h) != pdPASS) {
			ERROR_MSG("%s: failed to attach native task.\n", __func__);
			goto error;
		} else {
			INFO_MSG("%s: successfully attached native task.\n", __func__);
		}
	}
error:
	ERROR_MSG("Trivial migrator in error state, suspending.\n");
	while (1)
		vTaskSuspend(NULL);
}

int main()
{
	xTaskHandle h;
	setup_heap();

	/* if (!alloc_link_start_from_binary_register("tunnel")) */
	/* 	goto error; */

	if (xTaskCreate(native_task, (const signed char *)"native_task", configMINIMAL_STACK_SIZE, NULL, 3, &h) != pdPASS) {
		ERROR_MSG("Could not create native task.\n");
		goto error;
	}

	if (xTaskDetach(h) != pdPASS) {
		ERROR_MSG("Could not detach native task before scheduler started.\n");
		goto error;
	} else {
		INFO_MSG("Successfully detached native task.\n");
	}

	if (xTaskCreate(trivial_migrator, (const signed char *)"trivial_migrator", configMINIMAL_STACK_SIZE, h, 4, NULL) != pdPASS) {
		ERROR_MSG("Could not create trivial migrator task.\n");
		goto error;
	}

	if (xTaskAttach(h) != pdPASS) {
		ERROR_MSG("Could not attach native task before scheduler started.\n");
		goto error;
	} else {
		INFO_MSG("Successfully attached native task.\n");
	}

	INFO_MSG("%s: Task handle = 0x%x\n", __func__, (npi_t)h);

	DEBUG_MSG("Starting scheduler.\n");
	vTaskStartScheduler();

error:
	ERROR_MSG("no scheduler\n");
	while (1)
		;
	return 0;

}

void vApplicationMallocFailedHook( void )
{
	printf("Malloc failed\n");
	__asm volatile (" smc #0 ");
}

void vApplicationStackOverflowHook( void )
{
	printf("Task stack overflow.\n");
	while (1)
		;
}

void vApplicationIdleHook( void )
{

}
