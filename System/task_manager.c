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

#define TASK_MANAGER TASK_MANAGER
#define SYSTEM_MODULE TASK_MANAGER

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

task_register_tree task_register_tree_var =
  RB_INITIALIZER(task_register_tree);

SPLAY_GENERATE(task_dynmemsect_tree_t, task_dynmemsect_cons_t, dynmemsects, task_dynmemsect_cons_cmp)
RB_GENERATE(task_register_tree_t, task_register_cons_t, tasks, task_register_cons_cmp)

int get_number_of_tasks()
{
	int i = 0;
	task_register_cons *p;

	TASK_ACQUIRE_TR_LOCK();

	RB_FOREACH(p, task_register_tree_t, &task_register_tree_var) {
		i++;
	}

	TASK_RELEASE_TR_LOCK();

	return i;
}

task_register_tree *task_get_trc_root()
{
	return &task_register_tree_var;
}

task_register_cons *task_find(const char *name)
{
	struct task_register_cons_t *p;

	TASK_ACQUIRE_TR_LOCK();

	RB_FOREACH(p, task_register_tree_t, &task_register_tree_var) {
		if (strcmp(name, p->name) == 0)
			break;
	}

	TASK_RELEASE_TR_LOCK();

	return p;
}

void *task_get_section_address(task_register_cons *trc, Elf32_Half index)
{
	Elf32_Shdr *section_hdr = (Elf32_Shdr *)((u_int32_t)trc->elfh + trc->elfh->e_shoff);
	struct task_section_cons_t *p;

	if (index > trc->elfh->e_shnum)
		return NULL;

	if (section_hdr[index].sh_flags & SHF_ALLOC) {
		/*
		 * The section should be allocated.
		 */
		LIST_FOREACH(p, &trc->sections, sections) {
			if (p->section_index == index)
				return p->amem;
		}
	} else {
		/*
		 * If it is not an allocated section, return the
		 * address to the elf binary.
		 */
		return (void *)((u_int32_t)trc->elfh + section_hdr[index].sh_offset);
	}
	/*
	 * Did not find section.
	 */
	return NULL;
}


int task_link(task_register_cons *trc)
{
	if (check_elf_magic(trc->elfh))
		INFO_MSG("ELF magic checks out for task \"%s\".\n", trc->name);
	else {
		ERROR_MSG("Wrong ELF magic for task \"%s\".\n", trc->name);
		return 0;
	}

	if (trc->cont_mem == NULL) {
		ERROR_MSG("No memory allocated for task \"%s\" when trying to link.\n", trc->name);
		return 0;
	}

	Elf32_Ehdr *sys_elfh  = SYSTEM_ELF;

	/*
	 * The system elf could esstially come from anywhere so we
	 * will check the magic each time.
	 */
	DEBUG_MSG("System elf @ 0x%x\n", (npi_t)sys_elfh);
	if (!check_elf_magic(sys_elfh)) {
		ERROR_MSG("System elf magic does not check out.\n");
		return 0;
	}

	if (link_relocations(trc, sys_elfh, &task_register_tree_var)) {
		INFO_MSG("Relocation successful\n");
	} else {
		ERROR_MSG("Relocation failed\n");
		return 0;
	}

	return 1;
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

	/*
	 * Use TASKSECTION_MALLOC_CALL() for allocating task memory.
	 */

	u_int32_t cm_addr = (u_int32_t)TASKSECTION_MALLOC_CALL(alloc_size);

	if (cm_addr == 0) {
		ERROR_MSG("Could not allocate memory for task \"%s\"\n", trc->name);
		return 0;
	}

	trc->cont_mem	   = (void *)cm_addr;
	trc->cont_mem_size = alloc_size;

	LIST_INIT(&trc->sections);

	for (i = 0; i < trc->elfh->e_shnum; i++) {
		if (s[i].sh_flags & SHF_ALLOC) {
			struct task_section_cons_t *tsc =
				(struct task_section_cons_t *)
				SYSTEM_MALLOC_CALL(sizeof(task_section_cons));

			if (tsc == NULL) {
				ERROR_MSG("Could not allocate memory for tsc when allocating memory for task \"%s\".\n", trc->name);
				return 0;
			}

			tsc->name =
				(char *)
				((u_int32_t)trc->elfh + (u_int32_t)strtab_sect->sh_offset +
				 s[i].sh_name);
			DEBUG_MSG("Processing allocation for section \"%s\".\n", tsc->name);
			tsc->section_index = i;
			tsc->amem = (void *)(cm_addr + s[i].sh_addr);
			LIST_INSERT_HEAD(&trc->sections, tsc, sections);

			if (s[i].sh_type != SHT_NOBITS) {
				/*
				 * Copy the section if it contains data.
				 */
				DEBUG_MSG("Copying data for section \"%s\".\n", tsc->name);
				memcpy(tsc->amem, (void *)((u_int32_t)trc->elfh +
							   (u_int32_t)s[i].sh_offset),
				       s[i].sh_size);
			} else {
				/*
				 * Set the entire section to zero.
				 */
				DEBUG_MSG("Setting data for section \"%s\" to zero.\n", tsc->name);
				bzero(tsc->amem, s[i].sh_size);
			}
		}
	}

	SPLAY_INIT(&trc->dynmemsects);

	trc->request_hook = migrator_find_request_hook(trc);

#ifdef DATA_CACHE_ENABLED
	vPortCleanDataCache();
#endif /* DATA_CACHE_ENABLED */

	return 1;
}

int task_free(task_register_cons *trc)
{
	struct task_section_cons_t *p, *p_tmp;

	/*
	 * 1. Free each cons in the section list.
	 */

	LIST_FOREACH_SAFE(p, &trc->sections, sections, p_tmp) {
		LIST_REMOVE(p, sections);
		SYSTEM_FREE_CALL(p);
	}

	/*
	 * 2. Free all dynamically allocated memory sections.
	 *    Comment: O(n) algorithm looks a little nasty.
	 */

	task_dynmemsect_cons *dms_start, *dms_p, *dms_q;

	dms_start = SPLAY_MIN(task_dynmemsect_tree_t, &trc->dynmemsects);

	for (dms_p = dms_start; dms_p != NULL; dms_p = dms_q) {
		dms_q = SPLAY_NEXT(task_dynmemsect_tree_t, &trc->dynmemsects, dms_p);
		SYSTEM_FREE_CALL(dms_p->ptr);
		dms_p->ptr = dms_q;
	}

	for (dms_p = dms_start; dms_p != NULL; dms_p = dms_p->ptr) {
		SYSTEM_FREE_CALL(dms_p);
	}

	/*
	 * 3. Free the continous memory region. This is a TASKSECTION
	 *    so we are using the correct CALL.
	 */

	TASKSECTION_FREE_CALL(trc->cont_mem);
	trc->cont_mem	   = NULL;
	trc->cont_mem_size = 0;

	return 1;
}

int task_start(task_register_cons *trc)
{
	Elf32_Sym *entry_sym = find_symbol("_start", trc->elfh);

	if (entry_sym == NULL) {
		ERROR_MSG("Could not find entry symbol for task \"%s\"\n", trc->name);
		return 0;
	}

	entry_ptr_t entry_point = trc->cont_mem + entry_sym->st_value;

	if (entry_sym != NULL)
		INFO_MSG("Found entry sym for task \"%s\"\n", trc->name);
	else {
		ERROR_MSG("Did not find entry sym for task \"%s\"\n", trc->name);
		return 0;
	}

	/*
	 * This hold at least for ARM.
	 */

	if ((u_int32_t)entry_sym & 0x3) {
		ERROR_MSG("the entry point (@ 0x%x) is not 4 byte aligned.\n", (u_int32_t)entry_sym);
		return 0;
	}

	DEBUG_MSG("Entry point for task \"%s\" = 0x%x\n", trc->name, (u_int32_t)entry_point);

	if (xTaskCreate((pdTASK_CODE)entry_point, (const signed char *)trc->name,
			configMINIMAL_STACK_SIZE, NULL,
			APPLICATION_TASK_PRIORITY, &trc->task_handle) != pdPASS) {
		ERROR_MSG("Could not create task for trc \"%s\".\n", trc->name);
		return 0;
	}

	task_register_tree *root = task_get_trc_root();

	TASK_ACQUIRE_TR_LOCK();

	RB_REMOVE(task_register_tree_t, root, trc);
	RB_INSERT(task_register_tree_t, root, trc);

	TASK_RELEASE_TR_LOCK();

	DEBUG_MSG("Returning from task_start()\n");

	return 1;
}

task_register_cons *task_register(const char *name, Elf32_Ehdr *elfh)
{
	struct task_register_cons_t *trc =
		(task_register_cons *)SYSTEM_MALLOC_CALL(sizeof(task_register_cons));
	if (trc == NULL) {
		return NULL;
	}

	trc->name = name;
	trc->elfh = elfh;
	trc->task_handle = 0;
	TASK_ACQUIRE_TR_LOCK();
	RB_INSERT(task_register_tree_t, &task_register_tree_var, trc);
	TASK_RELEASE_TR_LOCK();

	trc->request_hook = NULL;
	trc->cont_mem = NULL;
	trc->cont_mem_size = 0;

	LIST_INIT(&trc->sections);
	SPLAY_INIT(&trc->dynmemsects);

	DEBUG_MSG("Number of tasks registered: %i\n", get_number_of_tasks());

	return trc;
}

task_dynmemsect_cons *task_find_dynmemsect(task_register_cons *trc, void *p)
{
	task_dynmemsect_cons criterion = { .ptr = p };
	SPLAY_SPLAY(task_dynmemsect_tree_t, &trc->dynmemsects, &criterion);

	task_dynmemsect_cons	*tdc	       = SPLAY_ROOT(&trc->dynmemsects);
	u_int32_t		 start_address = (u_int32_t)tdc->ptr;
	u_int32_t		 end_address   = start_address + tdc->size;
	u_int32_t		 address       = (u_int32_t)p;
	if (address >= start_address && address < end_address)
		return tdc;
	else
		return NULL;
}

void *task_apptask_malloc(size_t size, task_register_cons *trc)
{
	void *alloc_ptr = APPTASK_MALLOC_CALL(size);
	if (alloc_ptr == NULL)
		return NULL;
	task_dynmemsect_cons *dms = SYSTEM_MALLOC_CALL(sizeof(task_dynmemsect_cons));
	if (dms == NULL) {
		APPTASK_FREE_CALL(alloc_ptr);
		return NULL;
	}
	dms->ptr  = alloc_ptr;
	dms->size = size;

	SPLAY_INSERT(task_dynmemsect_tree_t, &trc->dynmemsects, dms);
	return alloc_ptr;
}

void task_apptask_free(void *ptr, task_register_cons *trc)
{
	APPTASK_FREE_CALL(ptr);
	task_dynmemsect_cons criterion = { .ptr = ptr };
	task_dynmemsect_cons *dms =
		SPLAY_FIND(task_dynmemsect_tree_t,
			   &trc->dynmemsects, &criterion);
	if (dms != NULL) {
		SPLAY_REMOVE(task_dynmemsect_tree_t, &trc->dynmemsects, dms);
		SYSTEM_FREE_CALL(dms);
	}
}

void *apptask_malloc(size_t size)
{
	task_register_tree	*root	   = task_get_trc_root();
	task_register_cons	 criterion = { .task_handle = xTaskGetCurrentTaskHandle() };
	task_register_cons	*trc	   = RB_FIND(task_register_tree_t, root, &criterion);

	/*
	 * If the task is not found in the register we should not
	 * bother allocating any memory.
	 */

	if (trc == NULL)
		return NULL;

	return task_apptask_malloc(size, trc);
}

void apptask_free(void *ptr)
{
	task_register_tree	*root	   = task_get_trc_root();
	task_register_cons	 criterion = { .task_handle = xTaskGetCurrentTaskHandle() };
	task_register_cons	*trc	   = RB_FIND(task_register_tree_t, root, &criterion);

	/*
	 * If the task is not found in the register we should not free
	 * any memory.
	 */

	if (trc == NULL)
		return;

	return task_apptask_free(ptr, trc);
}
