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

#include "FreeRTOS.h"

#include "task.h"
#include "queue.h"

#include "types.h"

#include "linker.h"
#include "applications.h"
#include "migrator.h"

#include <stdio.h>
#include <string.h>

#include "pl011.h"

#include <elf.h>


int link_and_start(Elf32_Ehdr *elfh, Elf32_Ehdr *sys_elfh, Elf32_Ehdr **other_elfhs, const char *name)
{

	if (check_elf_magic(elfh))
		printf("ELF magic checks out for task \"%s\".\n", name);
	else {
		printf("Wrong ELF magic for task \"%s\".\n", name);
		return 0;
	}

	Elf32_Sym *entry_sym = find_symbol("_start", elfh);

	if (entry_sym != NULL)
		printf("Found entry sym for task \"%s\"\n", name);
	else {
		printf("Did not find entry sym for task \"%s\"\n", name);
		return 0;
	}

	if (link_relocations(elfh, sys_elfh, other_elfhs)) {
		printf("Relocation successful\n");
	} else {
		printf("Relocation failed\n");
		return 0;
	}

	entry_ptr_t entry_point = get_entry_point(elfh, entry_sym);
	xTaskHandle th;

	xTaskCreate((pdTASK_CODE)entry_point, (const signed char *)name,
		    configMINIMAL_STACK_SIZE, NULL,
		    APPLICATION_TASK_PRIORITY, &th);

	if (!migrator_register(name, elfh, th)) {
		printf("Failed to register task \"%s\" to the migrator\n", name);
		return 0;
	}


	return 1;
}

int main()
{
	Elf32_Ehdr *simple_elfh = (Elf32_Ehdr *)&_simple_elf_start;
	Elf32_Ehdr *writer_elfh = (Elf32_Ehdr *)&_writer_elf_start;
	Elf32_Ehdr *reader_elfh = (Elf32_Ehdr *)&_reader_elf_start;
	Elf32_Ehdr *rtuapp_elfh = (Elf32_Ehdr *)&_rtuappv1_elf_start;

	Elf32_Ehdr *elfhs[4];

	elfhs[0] = simple_elfh;
	elfhs[1] = writer_elfh;
	elfhs[2] = reader_elfh;
	elfhs[3] = NULL;

	Elf32_Ehdr *sys_elfh = (Elf32_Ehdr *)&_system_elf_start;

	if (check_elf_magic(sys_elfh))
		printf("System ELF magic checks out @ 0x%x\n", (u_int32_t)sys_elfh);
	else {
		printf("Wrong System ELF magic @ 0x%x\n", (u_int32_t)sys_elfh);
		goto exit;
	}

	/*
	 * Link and start application tasks.
	 */

	link_and_start(simple_elfh, sys_elfh, elfhs, "simple");
	//link_and_start(reader_elfh, sys_elfh, elfhs, "reader");
	//link_and_start(writer_elfh, sys_elfh, elfhs, "writer");
	link_and_start(rtuapp_elfh, sys_elfh, elfhs, "rtuapp");

	/*
	 * Create migration task.
	 */

	if (!migrator_start()) {
		printf("ERROR: Could not start migrator.\n");
	}

	printf("Starting scheduler\n");
	vTaskStartScheduler();

exit:
	printf("Going into infinite loop...\n");
	while(1)
		;
}

void vApplicationMallocFailedHook( void )
{
	__asm volatile (" smc #0 ");
}
