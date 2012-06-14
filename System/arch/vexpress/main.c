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

#include <stdio.h>

#include <System/types.h>
#include <System/arch/vexpress/memory_layout.h>
#include <System/umm/umm_malloc.h>

portTASK_FUNCTION_PROTO(ohai_task, arg);

#define SYS_CFGDATA (*(volatile uint32_t *)0x100000a0)
#define SYS_CFGCTRL (*(volatile uint32_t *)0x100000a4)
#define SYS_CFGSTAT (*(volatile uint32_t *)0x100000a8)

typedef unsigned int uint32_t;

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
	vTaskDelay(10*portCORE_ID());
	printf("O hai! (%u)\n", (unsigned int)portCORE_ID());
	while (1) {
		printf("O hai! (%u), power = %i\n", (unsigned int)portCORE_ID(), GetCortexPower());
		vTaskDelay(100);
	}

}

int main()
{
	xMemoryInformationType *mit = MIS_ADDRESS;

	printf("Kernel @ core #%u.\n", (unsigned int)portCORE_ID());

	void	*heap	   = mit[portCORE_ID()].phys_heap_begin;
	size_t	 heap_size = mit[portCORE_ID()].phys_heap_size;

	printf("Heap @ 0x%x, heap size = %u\n", (npi_t)heap, heap_size);

	umm_init(heap, heap_size);

	void *a = pvPortMalloc(4000);
	void *b = pvPortMalloc(4000);

	printf("a = 0x%x, b = 0x%x\n", (npi_t)a, (npi_t)b);

	umm_info(a,1000);

	xTaskCreate(ohai_task, (const signed char *)"ohai", configMINIMAL_STACK_SIZE, NULL,
		    2, NULL);
	vTaskStartScheduler();
	printf("ERROR: main() exit\n");
	return 0;
}

void vApplicationMallocFailedHook( void )
{
	printf("Malloc failed\n");
	__asm volatile (" smc #0 ");
}

void vApplicationIdleHook( void )
{

}

