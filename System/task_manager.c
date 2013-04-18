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

#define SYSTEM_MODULE_NAME "TASK_MANAGER"

#include <task.h>
#include <semphr.h>

#include <types.h>
#include <linker.h>
#include <migrator.h>
#include <applications.h>
#include <rtu.h>
#include <system.h>
#include <task_manager.h>

#include <elf.h>

#include <stdio.h>
#include <string.h>

task_register_cons *task_register_list = NULL;

int get_number_of_tasks()
{
	int i = 0;
	task_register_cons *p = task_register_list;
	while (p) {
		i++;
		p = p->next;
	}
	return i;
}

int get_elf_array(Elf32_Ehdr **array, int array_size)
{
	int i;
	task_register_cons *p = task_register_list;
	for (i = 0; i < array_size; i++) {
		if (p == NULL) {
			array[i] = NULL;
			continue;
		}
		p = p->next;
		array[i] = p->elfh;
	}
	return 1;
}

task_register_cons *task_find(const char *name)
{
	task_register_cons *p = task_register_list;
	while (p) {
		if (strcmp(name, p->name) == 0)
			break;
		p = p->next;
	}
	return p;
}

int task_link(task_register_cons *trc)
{
	if (check_elf_magic(trc->elfh))
		INFO_MSG("ELF magic checks out for task \"%s\".\n", trc->name);
	else {
		ERROR_MSG("Wrong ELF magic for task \"%s\".\n", trc->name);
		return 0;
	}
	
	int elf_array_size = get_number_of_tasks() + 1;
	Elf32_Ehdr *elf_array[elf_array_size];
	get_elf_array(elf_array, elf_array_size);

	Elf32_Ehdr *sys_elfh  = (Elf32_Ehdr *)&_system_elf_start;
	
	if (link_relocations(trc->elfh, sys_elfh, elf_array)) {
		INFO_MSG("Relocation successful\n");
	} else {
		ERROR_MSG("Relocation failed\n");
		return 0;
	}

	return 1;
}

int task_start(task_register_cons *trc)
{
	Elf32_Sym *entry_sym = find_symbol("_start", trc->elfh);
	
	entry_ptr_t entry_point = get_entry_point(trc->elfh, entry_sym);

	if (entry_sym != NULL) 
		INFO_MSG("Found entry sym for task \"%s\"\n", trc->name);
	else {
		ERROR_MSG("Did not find entry sym for task \"%s\"\n", trc->name);
		return 0;
	}

	xTaskCreate((pdTASK_CODE)entry_point, (const signed char *)trc->name, 
		    configMINIMAL_STACK_SIZE, NULL, 
		    APPLICATION_TASK_PRIORITY, &trc->task_handle);
	return 1;
}

task_register_cons *task_register(const char *name, Elf32_Ehdr *elfh)
{
	task_register_cons *trc = 
		(task_register_cons *)pvPortMalloc(sizeof(task_register_cons));
	if (trc == NULL) {
		return NULL;
	}

	trc->name = name;
	trc->elfh = elfh;
	trc->task_handle = 0;
	trc->next = task_register_list;
	task_register_list = trc;

	Elf32_Sym *request_hook_symbol = find_symbol("cpRequestHook", elfh);
	if (request_hook_symbol) {
		INFO_MSG("Found request hook symbol in task \"%s\"\n", name);
		trc->request_hook = (request_hook_fn_t)((u_int32_t)elfh + (u_int32_t)request_hook_symbol->st_value);
	} else
		trc->request_hook = NULL;

	DEBUG_MSG("Number of tasks registered: %i\n", get_number_of_tasks());

	return trc;
}
