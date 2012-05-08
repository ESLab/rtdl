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
#include <semphr.h>

#include <types.h>
#include <linker.h>
#include <migrator.h>
#include <applications.h>
#include <rtu.h>
#include <system.h>

#include <elf.h>

#include <stdio.h>
#include <string.h>

typedef struct task_register_cons_t {
	const char *name;
	Elf32_Ehdr *elfh;
	xTaskHandle task_handle;
	request_hook_fn_t request_hook;
	struct task_register_cons_t *next;
} task_register_cons;

task_register_cons *task_register_list = NULL;

xTaskHandle      migrator_task_handle;
xSemaphoreHandle migrator_semaphore;

task_register_cons *find_register_cons(const char *name)
{
	task_register_cons *p = task_register_list;
	while (p) {
		if (strcmp(name, p->name) == 0)
			break;
		p = p->next;
	}
	return p;
}

void runtime_update(task_register_cons *rc, Elf32_Ehdr *new_sw)
{
	/*
	 * Make sure that task is suspended.
	 */
	vTaskSuspend(rc->task_handle);

	/*
	 * Check elf magic on the new software
	 */

	if (!check_elf_magic(new_sw)) {
		ERROR_MSG("MIGRATOR: elf magic on new software does not check out.\n");
		return;
	}

	Elf32_Sym *new_entry_sym = find_symbol("_start", new_sw);
	Elf32_Ehdr *sys_elfh = (Elf32_Ehdr *)&_system_elf_start;

	if (new_entry_sym == NULL) {
		ERROR_MSG("MIGRATOR: could not find entry symbol for new software for task \"%s\"\n", rc->name);
		return;
	}

	/*
	 * Link the new software.
	 */

	if (!link_relocations(new_sw, sys_elfh, NULL)) {
		ERROR_MSG("MIGRATOR: could not run-time link new software for task \"%s\"\n", rc->name);
		return;
	}

	/*
	 * Find the .rtu_data sections.
	 */

	Elf32_Shdr *old_rtu = find_section(RTU_DATA_SECTION_NAME, rc->elfh);
	Elf32_Shdr *new_rtu = find_section(RTU_DATA_SECTION_NAME, new_sw);

	/*
	 * Do some checks on the .rtu_data sections in the old and new elf.
	 */

	if (old_rtu == NULL || new_rtu == NULL) {
		ERROR_MSG("MIGRATOR: could not find \"" RTU_DATA_SECTION_NAME "\" sections in elfs\n");
		return;
	}

	if (old_rtu->sh_size != new_rtu->sh_size) {
		ERROR_MSG("MIGRATOR: size mismatch in \"" RTU_DATA_SECTION_NAME "\" sections between software versions.\n");
		return;
	}

	/*
	 * Copy the .rtu_data section from the old to the new
	 * software. We should here somewhere run the transformation
	 * function too.
	 */

	memcpy((void *)new_rtu->sh_addr, (void *)old_rtu->sh_addr, old_rtu->sh_size);

	/*
	 * Free the old software.
	 */

	free_relocations(rc->elfh);

	/*
	 * Delete old task, create new task and update register_cons.
	 */

	vTaskDelete(rc->task_handle);

	entry_ptr_t entry_point = get_entry_point(new_sw, new_entry_sym);

	rc->elfh = new_sw;
	Elf32_Sym *request_hook_symbol = find_symbol("cpRequestHook", new_sw);
	rc->request_hook = (request_hook_fn_t)((u_int32_t)new_sw + (u_int32_t)request_hook_symbol->st_value);
	
	xTaskCreate((pdTASK_CODE)entry_point, (const signed char *)rc->name,
		    configMINIMAL_STACK_SIZE, NULL,
		    APPLICATION_TASK_PRIORITY, &rc->task_handle);
	

}

void migrator_task(void *arg)
{
	task_register_cons *rc;

	while (1) {

		vTaskDelay(1000/portTICK_RATE_MS);
	
		if ((rc = find_register_cons("rtuapp"))) {
			xSemaphoreTake(migrator_semaphore, portMAX_DELAY);
			INFO_MSG("Calling request hook.\n");
			(rc->request_hook)(cp_req_rtu);
			INFO_MSG("Returned from request hook.\n");
			xSemaphoreTake(migrator_semaphore, portMAX_DELAY);
			DEBUG_MSG("BOO!\n");
			
			Elf32_Ehdr *new_sw = (Elf32_Ehdr *)&_rtuappv2_elf_start;
			
			INFO_MSG("Starting runtime update.\n");
			runtime_update(rc, new_sw);
			INFO_MSG("Runtime update complete.\n");
			xSemaphoreGive(migrator_semaphore);
		}

		vTaskDelay(1000/portTICK_RATE_MS);
	
		if ((rc = find_register_cons("rtuapp"))) {
			xSemaphoreTake(migrator_semaphore, portMAX_DELAY);
			INFO_MSG("Calling request hook.\n");
			(rc->request_hook)(cp_req_rtu);
			INFO_MSG("Returned from request hook.\n");
			xSemaphoreTake(migrator_semaphore, portMAX_DELAY);
			DEBUG_MSG("BOO!\n");
			
			Elf32_Ehdr *new_sw = (Elf32_Ehdr *)&_rtuappv1_elf_start;
			
			INFO_MSG("Starting runtime update.\n");
			runtime_update(rc, new_sw);
			INFO_MSG("Runtime update complete.\n");
			xSemaphoreGive(migrator_semaphore);
		}
	}
}

int migrator_register(const char *name, Elf32_Ehdr *elfh, xTaskHandle task_handle)
{
	task_register_cons *trc = 
		(task_register_cons *)
		pvPortMalloc(sizeof(task_register_cons));
	if (trc == NULL) {
		return 0;
	}
	
	trc->name = name;
	trc->elfh = elfh;
	trc->task_handle = task_handle;
	trc->next = task_register_list;
	task_register_list = trc;
	
	Elf32_Sym *request_hook_symbol = find_symbol("cpRequestHook", elfh);
	trc->request_hook = (request_hook_fn_t)((u_int32_t)elfh + (u_int32_t)request_hook_symbol->st_value);

	return 1;
}

int migrator_start()
{
	xTaskCreate(migrator_task, (const signed char *)"migrator", 
		    configMINIMAL_STACK_SIZE, NULL, 3, &migrator_task_handle);

	vSemaphoreCreateBinary(migrator_semaphore);
	DEBUG_MSG("migrator_semaphore @ 0x%x\n", (u_int32_t)&migrator_semaphore);

	DEBUG_MSG("migrator_task_handle @ 0x%x\n", (u_int32_t)&migrator_task_handle);

	return 1;
}
