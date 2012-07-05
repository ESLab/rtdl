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

#define SYSTEM_MODULE MAIN_RTUCONT

#include <task.h>
#include <queue.h>

#include <System/types.h>
#include <System/linker.h>
#include <System/applications.h>
#include <System/migrator.h>
#include <System/system.h>
#include <System/task_manager.h>
#include <System/pl011.h>
#include <System/pl111.h>
#include <System/elf.h>

#include <App/effects/field_effect.h>
#include <App/effects/Utils.h>
#include <App/rtucont/rtu_controller.h>

#include <stdio.h>
#include <string.h>

xQueueHandle		MeasurementQueue;
xQueueHandle		ControlSignalQueue;

volatile u_int32_t	fc = 0;
volatile portTickType	sl = 0;

portTASK_FUNCTION(plant_task, arg)
{
	effect_field_state	 effect_state;
	u_int16_t		*framebuffer1 = (u_int16_t *)0x4c000000;
#ifdef DBL_BUFFER
	u_int16_t		*framebuffer2 = (u_int16_t *)0x4c400000;
#endif /* DBL_BUFFER */

	InitializeScreen640x480(RGB16BitMode,framebuffer1);

	InitializeField(&effect_state, 640/4, 480/4, 640, 480,
			0,
			0);

	SetScreenFrameBuffer(framebuffer1);

	fc = 0;

	while (1) {
#ifdef DBL_BUFFER
		DrawField(&effect_state, framebuffer2);
		SetScreenFrameBuffer(framebuffer2);
		fc++;
#endif /* DBL_BUFFER */

		DrawField(&effect_state, framebuffer1);
		SetScreenFrameBuffer(framebuffer1);
		fc++;

		vTaskDelay(sl);
	}
}

portTASK_FUNCTION(frame_counting_task, arg)
{
	const u_int32_t funit	       = RTUCONT_FIXED_UNIT;
	const u_int32_t alpha	       = funit / 0x100;
	u_int32_t	old_fc;
	u_int32_t	new_fc	       = fc;
	u_int32_t	average	       = funit;
	portTickType	last_wake_time = xTaskGetTickCount();

	while(1) {
		old_fc = new_fc;
		vTaskDelayUntil(&last_wake_time, 1000/FRAME_COUNTING_FREQUENCY);
		new_fc = fc;

		u_int32_t	d   = new_fc - old_fc;
		average		    = alpha * d + (funit - alpha) * average;
		u_int32_t       fps = (funit * d) * FRAME_COUNTING_FREQUENCY;

		if (xQueueSendToBack(MeasurementQueue, (void *)&d, (portTickType)0) != pdPASS) {
			ERROR_MSG("Failed to send measurement to the controller\n");
		}

		controlsignal_t cs = 0;

		if (xQueueReceive(ControlSignalQueue, (void *)&cs, (portTickType)0) != pdPASS) {
			ERROR_MSG("Failed to receive control signal from the controller.\n");
			cs = 0;
		} else {
			sl = (u_int32_t)cs;
		}

		INFO_MSG("Fps: %d.%01d\n", fps / funit, fps % funit);
	}
}


static int setup_hardware( void )
{
	vUARTInitialise(portCORE_ID(), 38400, 64);
	vUARTInitialise(3, 38400, 64);
	return 1;
}

static int setup_queues( void )
{
	if ((MeasurementQueue = xQueueCreate(1, sizeof(measurement_t))) == 0) {
		ERROR_MSG("Could not create measurement queue.\n");
		return 0;
	}

	if ((ControlSignalQueue = xQueueCreate(1, sizeof(controlsignal_t))) == 0) {
		ERROR_MSG("Could not create control signal queue.\n");
		return 0;
	}

	DEBUG_MSG("Control signal @ 0x%x\n", (npi_t)&ControlSignalQueue);

	return 1;
}

static int start_dl_tasks( void )
{
	Elf32_Ehdr	*rtucont_elfh = APPLICATION_ELF(rtucontv1);
	Elf32_Ehdr	*sys_elfh     = SYSTEM_ELF;

	if (check_elf_magic(sys_elfh))
		INFO_MSG("System ELF magic checks out @ 0x%x\n", (u_int32_t)sys_elfh);
	else {
		ERROR_MSG("Wrong System ELF magic @ 0x%x\n", (u_int32_t)sys_elfh);	
		return 0;
	}

	task_register_cons *rtucontc = task_register("rtucont", rtucont_elfh);

	if (!task_alloc(rtucontc)) {
		ERROR_MSG("Could not alloc memory for task \"rtucont\"\n");
		return 0;
	}

	/*
	 * Linking tasks
	 */

	if (!task_link(rtucontc)) {
		ERROR_MSG("Could not link \"rtucont\" task\n");
		return 0;
	}

	/*
	 * Starting tasks
	 */
	
	if (!task_start(rtucontc)) {
		ERROR_MSG("Could not start \"rtucont\" task\n");
		return 0;
	}

	return 1;
}

int main()
{
	if (!setup_hardware()) {
		ERROR_MSG("Could not setup hardware.\n");
		goto error;
	}

	if (!setup_queues()) {
		ERROR_MSG("Could not setup queues.\n");
		goto error;
	}

	if (xTaskCreate(plant_task, (const signed char *)"plant_task",
			configMINIMAL_STACK_SIZE, NULL, 2, NULL) != pdPASS) {
		ERROR_MSG("Could not start up plant_task.\n");
		goto error;
	}

	if (xTaskCreate(frame_counting_task, (const signed char *)"frame_counting_task",
			configMINIMAL_STACK_SIZE, NULL, 3, NULL) != pdPASS) {
		ERROR_MSG("Could not start up frame_counting_task.\n");
		goto error;
	}

	if (!start_dl_tasks()) {
		ERROR_MSG("Could not start dynamically linked tasks.\n");
		goto error;
	}

#if 0
	/*
	 * Create migration task.
	 */

	if (!migrator_start()) {
		ERROR_MSG("Could not start migrator.\n");
		goto exit;
	}
#endif

	INFO_MSG("Starting scheduler\n");
	vTaskStartScheduler();

error:
	INFO_MSG("No scheduler, going into infinite loop...\n");
	while(1)
		;
}

void vApplicationMallocFailedHook( void )
{
	__asm volatile (" smc #0 ");
}

void vApplicationIdleHook( void )
{

}
