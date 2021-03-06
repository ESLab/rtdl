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

#include <stdio.h>
#include <string.h>

#include <System/types.h>
#include <System/system.h>
#include <System/task_manager.h>
#include <System/migrator.h>
#include <System/binary_register.h>
#include <System/arch/vexpress_vm/memory_layout.h>
#include <System/umm/umm_malloc.h>

portTASK_FUNCTION_PROTO(ohai_task, arg);

#define SYS_CFGDATA (*(volatile uint32_t *)0x100000a0)
#define SYS_CFGCTRL (*(volatile uint32_t *)0x100000a4)
#define SYS_CFGSTAT (*(volatile uint32_t *)0x100000a8)

typedef unsigned int uint32_t;

int migrator_loop()
{
	task_register_cons *trc;
	migration_struct	*ms    = (migration_struct *)data;
	//xTaskHandle migration_check_msg_handle;
	int i;

	vTaskDelay(1000 * portCORE_ID());

	while (1) {
		vTaskDelay(10000);
		if ((trc = task_find("field"))) {
			INFO_MSG("%s: Found tunnel task, migrating...\n", __func__);

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
			INFO_MSG("%s: Did not find tunnel task, waiting...\n", __func__);
		}
	}

}

static uint32_t ReadValue(int function,int site,int position,int device)

{
	int wait=10000;

	SYS_CFGSTAT=0;
	SYS_CFGCTRL=0x80000000|(function<<20)|(site<<16)|(position<<12)|device;
	// Set up a transfer.

	while((SYS_CFGSTAT&0x01)==0 && wait--!=0)//; // Wait for completed flag.

	if(SYS_CFGSTAT&0x02) return 0xffffffff;

	return SYS_CFGDATA;
}

static int GetCortexPower()
{
	uint32_t val=ReadValue(12,1,0,1);
	return val/10000;
}

portTASK_FUNCTION(ohai_task, arg)
{
	unsigned int		 cid  = portCORE_ID();
	xMemoryInformationType	*mit  = MIS_START_ADDRESS;
	iccs_layout		*iccs = (iccs_layout *)mit[cid].phys_iccs_begin;

	DEBUG_MSG("Rcu section @ 0x%x\n", (npi_t)iccs->rcu_section[cid]);

	vTaskDelay(10*portCORE_ID());
	printf("O hai! (%u)\n", (unsigned int)portCORE_ID());
	while (1) {
		vTaskDelay(1000);
	}

	ERROR_MSG("error somewhere...\n");
	while (1)
		;

}

static binary_register_entry *find_binary_register_entry(const char *name, binary_register_entry *bre)
{
	int i;
	for (i = 0; bre[i].binary_name != NULL; i++) {
		if (strcmp(name, bre[i].binary_name) == 0)
			return &bre[i];
	}
	return NULL;
}

int alloc_link_start_from_binary_register(const char *name)
{
	xMemoryInformationType	*mit = MIS_START_ADDRESS;
	binary_register_entry *bin_register = (binary_register_entry *)mit[portCORE_ID()].phys_binary_register_begin;
	binary_register_entry *bre = find_binary_register_entry(name, bin_register);

	if (bre == NULL) {
		ERROR_MSG("Could not find application \"%s\"\n", name);
		goto error0;
	}

	task_register_cons *trc = task_register(name, bre->elfh);

	if (trc == NULL) {
		ERROR_MSG("Could not register task \"%s\"\n", name);
		goto error0;
	}

	if (!task_alloc(trc)) {
		ERROR_MSG("Could not alloc memory for task \"%s\"\n", name);
		goto error0;
	}

	if (!task_link(trc)) {
		ERROR_MSG("Could not link task \"%s\"\n", name);
		goto error0;
	}

	if (!task_start(trc)) {
		ERROR_MSG("Could not start task \"%s\" \n", name);
		goto error0;
	}
	return 1;
error0:
	return 0;
}

int main()
{
	xMemoryInformationType	*mit = MIS_START_ADDRESS;

	INFO_MSG("Kernel @ core #%u.\n", (unsigned int)portCORE_ID());

	void	*heap	   = mit[portCORE_ID()].phys_heap_begin;
	size_t	 heap_size = mit[portCORE_ID()].phys_heap_size - 0x10000;

	INFO_MSG("Heap @ 0x%x, heap size = %u\n", (npi_t)heap, heap_size);

	umm_init(heap, heap_size);

	switch (portCORE_ID()) {
	case 0:
		if (!alloc_link_start_from_binary_register("tunnel", "tunnel", bin_register))
			goto error;
		break;
	}

	switch (portCORE_ID()) {
	case 0:
		if (!alloc_link_start_from_binary_register("field", "field", bin_register))
			goto error;
		break;
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

