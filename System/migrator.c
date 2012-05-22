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

#define SYSTEM_MODULE_NAME "MIGRATOR"

#include <task.h>
#include <semphr.h>

#include <System/types.h>
#include <System/linker.h>
#include <System/migrator.h>
#include <System/applications.h>
#include <System/system.h>
#include <System/task_manager.h>
#include <System/elf.h>

#include <App/rtu.h>

#include <stdio.h>
#include <string.h>

xTaskHandle      migrator_task_handle;
xSemaphoreHandle migrator_semaphore;

request_hook_fn_t migrator_find_request_hook(task_register_cons *trc)
{
	request_hook_fn_t ret = NULL;
	Elf32_Sym *request_hook_symbol = find_symbol("cpRequestHook", trc->elfh);
	if (request_hook_symbol) {
		ret = (request_hook_fn_t)((u_int32_t)trc->cont_mem + (u_int32_t)request_hook_symbol->st_value);
		INFO_MSG("Found request hook symbol in task \"%s\" @ 0x%x\n", trc->name, (u_int32_t)ret);
	}

	return ret;
}

int runtime_update(task_register_cons *trc, Elf32_Ehdr *new_sw)
{
	/*
	 * Make sure that task is suspended.
	 */
	vTaskSuspend(trc->task_handle);

	/*
	 * Check elf magic on the new software
	 */

	if (!check_elf_magic(new_sw)) {
		ERROR_MSG("MIGRATOR: elf magic on new software does not check out.\n");
		return 0;
	}

	/*
	 * Allocate memory for the new software.
	 */

	struct task_register_cons_t *new_trc =
	  (struct task_register_cons_t *)pvPortMalloc(sizeof(task_register_cons));

	if (new_trc == NULL) {
		ERROR_MSG("could not allocate memory while run-time updating task \"%s\"\n", trc->name);
		return 0;
	}

	new_trc->name = trc->name;
	new_trc->elfh = new_sw;
	new_trc->task_handle = 0;
	new_trc->cont_mem = NULL;

	LIST_INIT(&new_trc->sections);

	if (!task_alloc(new_trc)) {
		ERROR_MSG("Could not allocate memory when run-time updating task \"%s\"\n",
			  new_trc->name);
		vPortFree(new_trc);
		return 0;
	}

	/*
	 * Link the new software.
	 */


	if (!task_link(new_trc)) {
		ERROR_MSG("Could not link new software when doing run-time update on task \"%s\"\n",
			  new_trc->name);
		task_free(new_trc);
		vPortFree(new_trc);
		return 0;
	}

	/*
	 * Find the checkpoint request hook.
	 */

	new_trc->request_hook = migrator_find_request_hook(new_trc);

	if (new_trc->request_hook == NULL) {
		ERROR_MSG("Could not find checkpoint request hook when run-time updating task \"%s\"\n",
			  new_trc->name);
		task_free(new_trc);
		vPortFree(new_trc);
		return 0;
	}

	/*
	 * Find the .rtu_data sections.
	 */

	Elf32_Half old_rtu_ndx = find_section_index(RTU_DATA_SECTION_NAME, trc->elfh);
	Elf32_Half new_rtu_ndx = find_section_index(RTU_DATA_SECTION_NAME, new_trc->elfh);
	Elf32_Shdr *old_rtu = find_section(RTU_DATA_SECTION_NAME, trc->elfh);
	Elf32_Shdr *new_rtu = find_section(RTU_DATA_SECTION_NAME, new_trc->elfh);

	if (old_rtu_ndx == 0 || new_rtu_ndx == 0 || old_rtu == NULL || new_rtu == NULL) {
		ERROR_MSG("could not find \"" RTU_DATA_SECTION_NAME "\" sections in elfs\n");
		task_free(new_trc);
		vPortFree(new_trc);
		return 0;
	}


	if (old_rtu->sh_size != new_rtu->sh_size) {
		ERROR_MSG("size mismatch in \"" RTU_DATA_SECTION_NAME "\" sections between software versions.\n");
		task_free(new_trc);
		vPortFree(new_trc);
		return 0;
	}

	void *old_rtu_mem = task_get_section_address(trc, old_rtu_ndx);
	void *new_rtu_mem = task_get_section_address(new_trc, new_rtu_ndx);

	if (old_rtu_mem == NULL || new_rtu_mem == NULL) {
		ERROR_MSG("could not find allocated memory for section \"" RTU_DATA_SECTION_NAME "\".\n");
		task_free(new_trc);
		vPortFree(new_trc);
		return 0;
	}

	/*
	 * Copy the .rtu_data section from the old to the new
	 * software. We should here somewhere run the transformation
	 * function too.
	 */

	memcpy((void *)new_rtu_mem, (void *)old_rtu_mem, old_rtu->sh_size);

	/*
	 * Free the old software, delete old task, create new task,
	 * start it and update register_cons.
	 */

	task_register_tree *root = task_get_trc_root();

	RB_INSERT(task_register_tree_t, root, new_trc);

	/*
	 * BUG: the task should be free after the idle task has had
	 *      the chance to free the task_handle.
	 */
	if (!task_start(new_trc)) {
		ERROR_MSG("Could not start new task, going back to old.\n");
		RB_REMOVE(task_register_tree_t, root, new_trc);
		task_free(new_trc);
		vPortFree(new_trc);
		vTaskResume(trc->task_handle);
		return 0;
	}
	RB_REMOVE(task_register_tree_t, root, trc);
	vTaskDelete(trc->task_handle);
	task_free(trc);
	vPortFree(trc);
	return 1;
}

void migrator_task(void *arg)
{
	task_register_cons *trc;
	int i = 0;

	while (1) {
		i++;

		if (i == 3)
			printf("asd\n");

		vTaskDelay(1000/portTICK_RATE_MS);

		if ((trc = task_find("rtuapp"))) {
			xSemaphoreTake(migrator_semaphore, portMAX_DELAY);
			INFO_MSG("Calling request hook.\n");
			(trc->request_hook)(cp_req_rtu);
			INFO_MSG("Returned from request hook.\n");
			xSemaphoreTake(migrator_semaphore, portMAX_DELAY);
			DEBUG_MSG("BOO! (-> v2)\n");

			Elf32_Ehdr *new_sw = (Elf32_Ehdr *)&_rtuappv2_elf_start;

			INFO_MSG("Starting runtime update.\n");
			runtime_update(trc, new_sw);
			INFO_MSG("Runtime update complete. (-> v2)\n");
			xSemaphoreGive(migrator_semaphore);
		}

		vTaskDelay(1000/portTICK_RATE_MS);

		if ((trc = task_find("rtuapp"))) {
			xSemaphoreTake(migrator_semaphore, portMAX_DELAY);
			INFO_MSG("Calling request hook.\n");
			(*trc->request_hook)(cp_req_rtu);
			INFO_MSG("Returned from request hook.\n");
			xSemaphoreTake(migrator_semaphore, portMAX_DELAY);
			DEBUG_MSG("BOO! (-> v1)\n");

			Elf32_Ehdr *new_sw = (Elf32_Ehdr *)&_rtuappv1_elf_start;

			INFO_MSG("Starting runtime update.\n");
			runtime_update(trc, new_sw);
			INFO_MSG("Runtime update complete. (-> v1)\n");
			xSemaphoreGive(migrator_semaphore);
		}
	}
}

int migrator_start()
{
	if (xTaskCreate(migrator_task, (const signed char *)"migrator",
			configMINIMAL_STACK_SIZE, NULL, 4, &migrator_task_handle) != pdPASS) {
		ERROR_MSG("could not create migrator task\n");
		return 0;
	}

	vSemaphoreCreateBinary(migrator_semaphore);
	DEBUG_MSG("migrator_semaphore @ 0x%x\n", (u_int32_t)&migrator_semaphore);

	DEBUG_MSG("migrator_task_handle @ 0x%x\n", (u_int32_t)&migrator_task_handle);

	return 1;
}
