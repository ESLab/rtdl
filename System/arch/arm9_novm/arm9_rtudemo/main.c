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

#define MAIN MAIN
#define SYSTEM_MODULE MAIN

#include <task.h>
#include <queue.h>

#include <System/types.h>
#include <System/linker.h>
#include <System/applications.h>
#include <System/migrator.h>
#include <System/system.h>
#include <System/task_manager.h>
#include <System/pl011.h>
#include <System/elf.h>
#include <System/binary_register.h>

#include <System/arch/arm9_novm/memory_layout.h>

#include <stdio.h>
#include <string.h>

int migrator_loop()
{
	binary_register_entry	*bin_register = BINARY_REGISTER;
	binary_register_entry	*bre;
	task_register_cons	*trc;

	while (1) {
		vTaskDelay(1000/portTICK_RATE_MS);

		if ((trc = task_find("rtuapp"))) {

			bre = find_binary_register_entry("rtuappv2", bin_register);
			if (bre == NULL)
				goto error;

			if (!task_wait_for_checkpoint(trc, cp_req_rtu)) {
				ERROR_MSG("%s: Failed to reach rtu checkpoint for task \"%s\"\n",
					  __func__, trc->name);
				return 0;
			}

			Elf32_Ehdr *new_sw = bre->elfh;

			INFO_MSG("Starting runtime update.\n");
			if (!migrator_runtime_update(trc, new_sw)) {
				ERROR_MSG("Runtime updating failed.\n");
				return 0;
			}
			INFO_MSG("Runtime update complete. (-> v2)\n");
		} else {
			goto error;
		}

		vTaskDelay(1000/portTICK_RATE_MS);

		if ((trc = task_find("rtuapp"))) {

			bre = find_binary_register_entry("rtuappv1", bin_register);
			if (bre == NULL)
				goto error;

			if (!task_wait_for_checkpoint(trc, cp_req_rtu)) {
				ERROR_MSG("%s: Failed to reach rtu checkpoint for task \"%s\"\n",
					  __func__, trc->name);
				return 0;
			}

			Elf32_Ehdr *new_sw = bre->elfh;

			INFO_MSG("Starting runtime update.\n");
			if (!migrator_runtime_update(trc, new_sw)) {
				ERROR_MSG("Runtime updating failed.\n");
				return 0;
			}
			INFO_MSG("Runtime update complete. (-> v1)\n");
		} else {
			goto error;
		}
	}
error:
	ERROR_MSG("Migration in error state, entering infinite loop.\n");
	while (1)
		;
}

int main()
{
	binary_register_entry *bre = BINARY_REGISTER;

	vUARTInitialise(0, 38400, 0);

	Elf32_Ehdr *sys_elfh = SYSTEM_ELF;

	if (check_elf_magic(sys_elfh))
		INFO_MSG("System ELF magic checks out @ 0x%x\n", (u_int32_t)sys_elfh);
	else {
		ERROR_MSG("Wrong System ELF magic @ 0x%x\n", (u_int32_t)sys_elfh);
		goto exit;
	}

	if (!alloc_link_start_from_binary_register("simple", "simple", bre)) {
		ERROR_MSG("Failed to start task \"simple\" from binary register.\n");
		goto exit;
	}

	if (!alloc_link_start_from_binary_register("rtuapp", "rtuappv1", bre)) {
		ERROR_MSG("Failed to start task \"rtuapp\" from binary \"rtuappv1\".\n");
		goto exit;
	}

	/*
	 * Create migration task.
	 */

	if (!migrator_start()) {
		ERROR_MSG("Could not start migrator.\n");
		goto exit;
	}

	INFO_MSG("Starting scheduler\n");
	vTaskStartScheduler();
	goto exit;
exit:
	ERROR_MSG("Going into infinite loop...\n");
	while(1)
		;
}

void vApplicationMallocFailedHook( void )
{

}

void vApplicationIdleHook( void )
{

}
