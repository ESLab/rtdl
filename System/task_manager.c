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

#include <System/types.h>
#include <System/linker.h>
#include <System/migrator.h>
#include <System/applications.h>
#include <System/system.h>
#include <System/task_manager.h>
#include <System/elf.h>

#include <stdio.h>
#include <string.h>

#include <App/rtu.h>

SLIST_HEAD(task_register_list_t, task_register_cons_t) task_register_list =
	SLIST_HEAD_INITIALIZER(task_register_list);

int get_number_of_tasks()
{
	int i = 0;
	struct task_register_cons_t *p;

	SLIST_FOREACH(p, &task_register_list, tasks) {
		i++;
	}

	return i;
}

int get_elf_array(Elf32_Ehdr **array, int array_size)
{
	int i = 0;
	struct task_register_cons_t *p;

	SLIST_FOREACH(p, &task_register_list, tasks) {
		if (i > array_size)
			break;
		array[i++] = p->elfh;
	}

	for (; i < array_size; i++)
		array[i] = NULL;

	return 1;
}

task_register_cons *task_find(const char *name)
{
	struct task_register_cons_t *p;

	SLIST_FOREACH(p, &task_register_list, tasks) {
		if (strcmp(name, p->name) == 0)
			break;
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
int task_alloc(task_register_cons *trc)
{
	int i;
	Elf32_Shdr *s = (Elf32_Shdr *)((u_int32_t)trc->elfh + trc->elfh->e_shoff);
	u_int32_t alloc_size = 0;

	/*
	 * 1. Find out the size of the continous region that has to be
	 *    allocated.
	 */

	for (i = 0; i < trc->elfh->e_shnum; i++) {
		if (s[i].sh_flags & SHF_ALLOC) {
			u_int32_t s_req = s[i].sh_addr + s[i].sh_size;
			alloc_size = alloc_size > s_req ? alloc_size : s_req;
		}
	}
	DEBUG_MSG("memory required for task \"%s\": %u\n", trc->name, alloc_size);

	/*
	 * 2. Allocate continous memory region and create section
	 *    conses.
	 */

	Elf32_Shdr *section_hdr = (Elf32_Shdr *)((u_int32_t)trc->elfh + trc->elfh->e_shoff);
	Elf32_Shdr *strtab_sect = &section_hdr[trc->elfh->e_shstrndx];

	if (strtab_sect == NULL) {
		ERROR_MSG("Found no .strtab in elfh for task \"%s\"\n", trc->name);
		return 0;
	}

	u_int32_t cm_addr = (u_int32_t)pvPortMalloc(alloc_size);

	if (cm_addr == 0) {
		ERROR_MSG("Could not allocate memory for task \"%s\"\n", trc->name);
		return 0;
	}

	trc->cont_mem = (void *)cm_addr;

	SLIST_INIT(&trc->sections);

	for (i = 0; i < trc->elfh->e_shnum; i++) {
		if (s[i].sh_flags & SHF_ALLOC) {
			struct task_section_cons_t *tsc =
				(struct task_section_cons_t *)
				pvPortMalloc(sizeof(task_section_cons));
			tsc->name =
				(char *)
				((u_int32_t)trc->elfh + (u_int32_t)strtab_sect->sh_offset +
				 s[i].sh_name);
			DEBUG_MSG("Processing allocation for section \"%s\".\n", tsc->name);
			tsc->section_index = i;
			tsc->amem = (void *)(cm_addr + s[i].sh_addr);
			SLIST_INSERT_HEAD(&trc->sections, tsc, sections);

			if (s[i].sh_type == SHT_PROGBITS) {
				/*
				 * Copy the section if it contains data.
				 */
				memcpy(tsc->amem, (void *)((u_int32_t)trc->elfh +
							   (u_int32_t)s[i].sh_offset),
				       s[i].sh_size);
			}
		}
	}

	return 1;
}

int task_free(task_register_cons *trc)
{
	struct task_section_cons_t *p, *p_tmp;

	/*
	 * 1. Free each cons in the section list.
	 */

	SLIST_FOREACH_SAFE(p, &trc->sections, sections, p_tmp) {
		SLIST_REMOVE(&trc->sections, p, task_section_cons_t, sections);
		vPortFree(p);
	}

	/*
	 * 2. Free the continous memory region.
	 */

	vPortFree(trc->cont_mem);
	trc->cont_mem = NULL;

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
	struct task_register_cons_t *trc =
		(task_register_cons *)pvPortMalloc(sizeof(task_register_cons));
	if (trc == NULL) {
		return NULL;
	}

	trc->name = name;
	trc->elfh = elfh;
	trc->task_handle = 0;
	SLIST_INSERT_HEAD(&task_register_list, trc, tasks);

	Elf32_Sym *request_hook_symbol = find_symbol("cpRequestHook", elfh);
	if (request_hook_symbol) {
		INFO_MSG("Found request hook symbol in task \"%s\"\n", name);
		trc->request_hook = (request_hook_fn_t)((u_int32_t)elfh + (u_int32_t)request_hook_symbol->st_value);
	} else
		trc->request_hook = NULL;

	trc->cont_mem = NULL;

	SLIST_INIT(&trc->sections);

	DEBUG_MSG("Number of tasks registered: %i\n", get_number_of_tasks());

	return trc;
}
