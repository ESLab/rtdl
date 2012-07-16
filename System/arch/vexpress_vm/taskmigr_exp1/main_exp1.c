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

#define VX_EXPERIMENT1_MAIN VX_EXPERIMENT1_MAIN
#define SYSTEM_MODULE VX_EXPERIMENT1_MAIN

#include <FreeRTOS.h>

#include <task.h>
#include <queue.h>

#include <stdio.h>
#include <string.h>

#include <System/types.h>
#include <System/system.h>
#include <System/task_manager.h>
#include <System/migrator.h>
#include <System/arch/vexpress_vm/memory_layout.h>
#include <System/arch/vexpress_vm/binary_register.h>
#include <System/umm/umm_malloc.h>

#include <App/effects/config_effect.h>

const char *effect_name[] = { "effect00", "effect01",
			      "effect10", "effect11" };

#define EFFECT_W 128
#define EFFECT_H 96

int migrator_loop()
{
	task_register_cons	*trc;
	migration_struct	*ms = (migration_struct *)data;
	int			 i;

	vTaskDelay(1000 * portCORE_ID());

	while (1) {
		vTaskDelay(10000);
		if ((trc = task_find("field"))) {
#ifndef TAKE_MEASUREMENT
			INFO_MSG("%s: Found tunnel task, migrating...\n", __func__);
#endif /* TAKE_MEASUREMENT */

			if (!task_wait_for_checkpoint(trc, cp_req_tm)) {
				ERROR_MSG("%s: Failed to reach migration checkpoint for task \"%s\"\n",
					  __func__, trc->name);
				continue;
			}

			if (!task_detach(trc)) {
				ERROR_MSG("%s: Could not detach task.\n", __func__);
				continue;
			}

			ms->target_core_id = (portCORE_ID() + 1) % 4;
			ms->trc		   = trc;

		} else {
#ifndef TAKE_MEASUREMENT
			INFO_MSG("%s: Did not find tunnel task, waiting...\n", __func__);
#endif /* TAKE_MEASUREMENT */
		}
	}

}

void unknown_interrupt_handler(void *arg)
{
	unsigned int interrupt = (unsigned int)arg;
	ERROR_MSG("Unknown interrupt! (%u)\n", interrupt);
	return;
}

static void setup_hardware()
{
	void vPortInstallInterruptHandler( void (*vHandler)(void *), void *pvParameter, unsigned long ulVector, unsigned char ucEdgeTriggered, unsigned char ucPriority, unsigned char ucProcessorTargets );
}



int main()
{
	xMemoryInformationType	*mit = MIS_START_ADDRESS;

	setup_hardware();

	INFO_MSG("Kernel @ core #%u.\n", (unsigned int)portCORE_ID());

	void		*heap	   = mit[portCORE_ID()].phys_heap_begin;
	size_t		 heap_size = mit[portCORE_ID()].phys_heap_size - 0x10000;

	unsigned int	 i;
	unsigned int     j;

	INFO_MSG("Heap @ 0x%x, heap size = %u\n", (npi_t)heap, heap_size);

	umm_init(heap, heap_size);

	{

		i = (portCORE_ID() & 0x1) >> 0;
		j = (portCORE_ID() & 0x2) >> 1;


		const char *effects[]	  = { "tunnel", "field" };
		if (!effect_start_and_config(effect_name[2*i+j], effects[j == i],
					     EFFECT_W, EFFECT_H,
					     i*EFFECT_W, j*EFFECT_H)) {
			ERROR_MSG("Could not start effect \"%s\" in quadrand (%u,%u).\n", effects[j == i],  i, j);
			goto error;
		}
	}

	if (!migrator_start()) {
		ERROR_MSG("Could not start migrator.\n");
		goto error;
	}

	DEBUG_MSG("Starting scheduler\n");
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

