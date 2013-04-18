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

#define SYSTEM_MODULE_NAME "MAIN"

#include <task.h>
#include <queue.h>

#include <types.h>
#include <linker.h>
#include <applications.h>
#include <migrator.h>
#include <system.h>
#include <task_manager.h>
#include <pl011.h>

#include <elf.h>

#include <stdio.h>
#include <string.h>

int main()
{
	Elf32_Ehdr *simple_elfh = (Elf32_Ehdr *)&_simple_elf_start;
	Elf32_Ehdr *writer_elfh = (Elf32_Ehdr *)&_writer_elf_start;
	Elf32_Ehdr *reader_elfh = (Elf32_Ehdr *)&_reader_elf_start;
	Elf32_Ehdr *rtuapp_elfh = (Elf32_Ehdr *)&_rtuappv1_elf_start;

	Elf32_Ehdr *sys_elfh = (Elf32_Ehdr *)&_system_elf_start;

	if (check_elf_magic(sys_elfh))
		INFO_MSG("System ELF magic checks out @ 0x%x\n", (u_int32_t)sys_elfh);
	else {
		ERROR_MSG("Wrong System ELF magic @ 0x%x\n", (u_int32_t)sys_elfh);	
		goto exit;
	}

	/*
	 * Registering tasks
	 */

	task_register_cons *simplec = task_register("simple", simple_elfh);
	task_register_cons *readerc = task_register("reader", reader_elfh);
	task_register_cons *writerc = task_register("writer", writer_elfh);
	task_register_cons *rtuappc = task_register("rtuapp", rtuapp_elfh);

	/*
	 * Linking tasks
	 */

	if (!task_link(simplec)) {
		ERROR_MSG("Could not link \"simple\" task\n");
		goto exit;
	}
	if (!task_link(readerc)) {
		ERROR_MSG("Could not link \"reader\" task\n");
		goto exit;
	}
	if (!task_link(writerc)) {
		ERROR_MSG("Could not link \"writer\" task\n");
		goto exit;
	}
	if (!task_link(rtuappc)) {
		ERROR_MSG("Could not link \"rtuapp\" task\n");
		goto exit;
	}

	/*
	 * Starting tasks
	 */
	
	if (!task_start(simplec)) {
		ERROR_MSG("Could not start \"simple\" task\n");
		goto exit;
	}

	if (!task_start(readerc)) {
		ERROR_MSG("Could not start \"reader\" task\n");
		goto exit;
	}

	if (!task_start(writerc)) {
		ERROR_MSG("Could not start \"writer\" task\n");
		goto exit;
	}

	if (!task_start(rtuappc)) {
		ERROR_MSG("Could not start \"rtuapp\" task\n");
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

exit:
	INFO_MSG("Going into infinite loop...\n");
	while(1)
		;
}

void vApplicationMallocFailedHook( void )
{
	__asm volatile (" smc #0 ");
}
