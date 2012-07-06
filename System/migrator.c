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

#define MIGRATOR MIGRATOR
#define SYSTEM_MODULE MIGRATOR

#include <task.h>
#include <semphr.h>

#include <System/types.h>
#include <System/linker.h>
#include <System/migrator.h>
#include <System/applications.h>
#include <System/system.h>
#include <System/task_manager.h>
#include <System/elf.h>
#ifdef RTU_POINTER_TRACING
#include <System/pointer_tracer.h>
#include <System/system_util.h>
#endif /* RTU_POINTER_TRACING */

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

#ifdef RUNTIME_UPDATING
#ifdef RTU_POINTER_TRACING

#include <dwarf.h>

struct runtime_update_cb_arg_t {
	u_int32_t lower_bound;
	u_int32_t upper_bound;
};

static int runtime_update_die_cb(pt_pstate *pstate, Dwarf_Die die, void *arg)
{
	struct runtime_update_cb_arg_t	*bounds = arg;
	Dwarf_Die			 type_die;
	Dwarf_Half			 tag;
	Dwarf_Error			 err;
	int				 res;
	char				*name;

	res = dwarf_diename(die, &name, &err);
	if (res == DW_DLV_ERROR) {
		ERROR_MSG("runtime_update_die_cb(): dwarf_diename() fail.\n");
		return 0;
	}
	if (res == DW_DLV_NO_ENTRY) {
		name = "<no DW_AT_name attr>";
	}
	res = dwarf_tag(die, &tag, &err) ;
	if (res != DW_DLV_OK) {
		ERROR_MSG("runtime_update_die_cb(): dwarf_tag() fail.\n");
		return 0;
	}
	if (tag != DW_TAG_variable)
		return 1;
	if (dwarfif_get_type_die(pstate->dbg, die, &type_die)) {
		task_register_cons *trc = pstate->trc;
		void *p = dwarfif_find_static_var_address(trc, die);
		u_int32_t address = (u_int32_t)p;
		DEBUG_MSG("Found variable @ 0x%x, we have lower_bound = 0x%x and upper_bound = 0x%x\n",
			  address, bounds->lower_bound, bounds->upper_bound);
		if (p != NULL && address >= bounds->lower_bound && address < bounds->upper_bound) {
			const char *s;
			if (dwarf_get_TAG_name(tag, &s) != DW_DLV_OK) {
				ERROR_MSG("%s: Could not get tag name.\n", __func__);
				return 0;
			}
			DEBUG_MSG("Found static variable \"%s\" @ 0x%x, with type %s.\n", name, (u_int32_t)p, s);
			INFO_MSG("Found type for variable %s\n", name);

			if (!pt_trace_pointer(pstate, type_die, p)) {
				DEBUG_MSG("Pointer tracing failed.\n");
			} else {
				DEBUG_MSG("Pointer tracing succeeded.\n");
			}
		}
	} else {
		INFO_MSG("Could not get type die for variable, ignoring.\n");
	}
	return 1;
}

static void *migrator_rtu_get_new_relocation_address(pt_pstate *pstate, int32_t rtu_data_delta, void *var_p, void *section_p)
{
	/*
	 * Find the pointers section.
	 */

	task_dynmemsect_cons	criterion_a = { .ptr = section_p };
	pt_dyn_memsect		criterion_b = { .tdc_p = &criterion_a };

	pt_dyn_memsect	*dms   = RB_FIND(pt_dyn_memsect_tree_t, &pstate->included_memsects, &criterion_b);

	void	*new_section_p;
	size_t	 new_section_size;
	void	*new_p;

	if (dms == NULL) {
		/*
		 * We did not find a dynamic memory
		 * section, we will try to find out if
		 * the variable is in the static
		 * memory section.
		 */
		new_section_p    = pstate->trc->cont_mem;
		new_section_size = pstate->trc->cont_mem_size;
		new_p		 = (void *)((u_int32_t)var_p + rtu_data_delta);
	} else {
		/*
		 * We do have a dynamic section.
		 */
		new_section_p	 = dms->tdc_p->ptr;
		new_section_size = dms->tdc_p->size;
		new_p		 = (void *)((u_int32_t)var_p + dms->delta_p);
	}

	if (!util_pointer_in_section(new_section_p, new_section_size, var_p)) {
		/*
		 * Pointer not in supposed section.
		 */
		return NULL;
	}

	return new_p;
}

#if 0

/*
 * This is a usable but not at the moment needed implementation of the
 * get_new_relocation_address functionality.
 */

static void *migrator_get_new_relocation_address(pt_pstate *pstate, task_register_cons *new_trc, void *var_p, void *section_p)
{
	/*
	 * Find the pointers section.
	 */

	task_dynmemsect_cons	criterion_a = { .ptr = section_p };
	pt_dyn_memsect		criterion_b = { .tdc_p = &criterion_a };

	pt_dyn_memsect	*dms   = RB_FIND(pt_dyn_memsect_tree_t, &pstate->included_memsects, &criterion_b);

	int32_t static_delta_p = (u_int32_t)new_trc->cont_mem - (u_int32_t)pstate->trc->cont_mem;

	void	*new_section_p;
	size_t	 new_section_size;
	void	*new_p;

	if (dms == NULL) {
		/*
		 * We did not find a dynamic memory
		 * section, we will try to find out if
		 * the variable is in the static
		 * memory section.
		 */
		new_section_p    = pstate->trc->cont_mem;
		new_section_size = pstate->trc->cont_mem_size;
		new_p		 = (void *)((u_int32_t)var_p + static_delta_p);
	} else {
		/*
		 * We do have a dynamic section.
		 */
		new_section_p	 = dms->tdc_p->ptr;
		new_section_size = dms->tdc_p->size;
		new_p		 = (void *)((u_int32_t)var_p + dms->delta_p);
	}

	if (!util_pointer_in_section(new_section_p, new_section_size, var_p)) {
		/*
		 * Pointer not in supposed section.
		 */
		return NULL;
	}

	return new_p;
}

#endif /* 0 */

#endif /* RTU_POINTER_TRACING */

int runtime_update(task_register_cons *trc, Elf32_Ehdr *new_sw)
{
	/*
	 * Make sure that task is suspended.
	 */
	vTaskSuspend(trc->task_handle);

	/*
	 * Check elf magic on the new software.
	 */

	if (!check_elf_magic(new_sw)) {
		ERROR_MSG("MIGRATOR: elf magic on new software does not check out.\n");
		goto error_L0;
	}

	/*
	 * Allocate memory for the new software.
	 */

	task_register_cons *new_trc =
		(task_register_cons *)SYSTEM_MALLOC_CALL(sizeof(task_register_cons));

	if (new_trc == NULL) {
		ERROR_MSG("could not allocate memory while run-time updating task \"%s\"\n", trc->name);
		return 0;
	}
	/*
	 * -> Error level 1.
	 */

	new_trc->name	       = trc->name;
	new_trc->elfh	       = new_sw;
	new_trc->task_handle   = 0;
	new_trc->cont_mem      = NULL;
	new_trc->cont_mem_size = 0;

	LIST_INIT(&new_trc->sections);

	if (!task_alloc(new_trc)) {
		ERROR_MSG("Could not allocate memory when run-time updating task \"%s\"\n",
			  new_trc->name);
		goto error_L1;
	}

	/*
	 * -> Error level 2.
	 */

	/*
	 * Link the new software.
	 */

	if (!task_link(new_trc)) {
		ERROR_MSG("Could not link new software when doing run-time update on task \"%s\"\n",
			  new_trc->name);
		goto error_L2;
	}

	/*
	 * Find the checkpoint request hook.
	 */

	new_trc->request_hook = migrator_find_request_hook(new_trc);

	if (new_trc->request_hook == NULL) {
		ERROR_MSG("Could not find checkpoint request hook when run-time updating task \"%s\"\n",
			  new_trc->name);
		goto error_L2;
	}

	/*
	 * Find the .rtu_data sections.
	 */

	Elf32_Half	 old_rtu_ndx = find_section_index(RTU_DATA_SECTION_NAME, trc->elfh);
	Elf32_Half	 new_rtu_ndx = find_section_index(RTU_DATA_SECTION_NAME, new_trc->elfh);
	Elf32_Shdr	*old_rtu     = find_section(RTU_DATA_SECTION_NAME, trc->elfh);
	Elf32_Shdr	*new_rtu     = find_section(RTU_DATA_SECTION_NAME, new_trc->elfh);

	if (old_rtu_ndx == 0 || new_rtu_ndx == 0 || old_rtu == NULL || new_rtu == NULL) {
		ERROR_MSG("could not find \"" RTU_DATA_SECTION_NAME "\" sections in elfs\n");
		goto error_L2;
	}

	if (old_rtu->sh_size != new_rtu->sh_size) {
		ERROR_MSG("size mismatch in \"" RTU_DATA_SECTION_NAME "\" sections between software versions.\n");
		goto error_L2;
	}

	void *old_rtu_mem = task_get_section_address(trc, old_rtu_ndx);
	void *new_rtu_mem = task_get_section_address(new_trc, new_rtu_ndx);

	if (old_rtu_mem == NULL || new_rtu_mem == NULL) {
		ERROR_MSG("could not find allocated memory for section \"" RTU_DATA_SECTION_NAME "\".\n");
		goto error_L2;
	}

#ifdef RTU_POINTER_TRACING

	struct runtime_update_cb_arg_t cb_arg =
		{ .lower_bound		      = (u_int32_t)trc->cont_mem + old_rtu->sh_addr,
		  .upper_bound		      = (u_int32_t)trc->cont_mem + old_rtu->sh_addr + old_rtu->sh_size };
	Dwarf_Debug	dbg;
	pt_pstate	pstate;

	if (!dwarfif_init(trc, &dbg)) {
		ERROR_MSG("Could not get debug object.\n");
		goto error_L2;
	}

	/*
	 * Trace pointers in the .rtu_data section.
	 */

	if (!pt_pstate_init(&pstate, dbg, trc)) {
		ERROR_MSG("pt_pstate_init() fail.\n");
		goto error_L2;
	}

	/*
	 * -> error_L2_rtu0
	 */

	/*
	 * We need the static memory section for pointer tracing.
	 */

	DEBUG_MSG("Tracing pointers...\n");

	if (!pt_iterate_dies(&pstate, runtime_update_die_cb, &cb_arg)) {
		ERROR_MSG("pt_iterate_dies() fail.\n");
		goto error_L2_rtu0;
	} else {
		DEBUG_MSG("Pointer tracing successful.\n");
	}

	/*
	 * Allocate new dynamic memory section, copy the section from
	 * the old to the new, and calculate pointer deltas.
	 */

	{
		pt_dyn_memsect *dmsp;

		RB_FOREACH(dmsp, pt_dyn_memsect_tree_t, &pstate.included_memsects) {
			u_int32_t	new_p = (u_int32_t)task_apptask_malloc(dmsp->tdc_p->size, new_trc);
			u_int32_t	old_p = (u_int32_t)dmsp->tdc_p->ptr;

			if (new_p == 0) {
				ERROR_MSG("Error allocating dynamic memory section with size %u.\n", dmsp->tdc_p->size);
				goto error_L2_rtu0;
			}

			memcpy((void *)new_p, (void *)old_p, dmsp->tdc_p->size);

			dmsp->delta_p = new_p - old_p;
			DEBUG_MSG("Copied %u bytes from 0x%x to 0x%x, delta_p = %i\n",
				  dmsp->tdc_p->size, old_p, new_p, dmsp->delta_p);
		}
	}

#endif /* RTU_POINTER_TRACING */

	/*
	 * Copy the .rtu_data section from the old to the new
	 * software. We should here somewhere run the transformation
	 * function too.
	 */

	memcpy((void *)new_rtu_mem, (void *)old_rtu_mem, old_rtu->sh_size);

#ifdef RTU_POINTER_TRACING

	/*
	 * For each visited pointer variable, update the pointed-to
	 * address.
	 */

	int32_t rtu_data_delta = (u_int32_t)new_rtu_mem - (u_int32_t)old_rtu_mem;

	{
		pt_visited_variable	*vvp;

		RB_FOREACH(vvp, pt_visited_variable_tree_t, &pstate.visited_variables) {
			Dwarf_Error	err;
			Dwarf_Half	tag;

			if (dwarf_tag(vvp->type_die, &tag, &err) != DW_DLV_OK) {
				ERROR_MSG("Could not get tag for type_die.\n");
				goto error_L2_rtu0;
			}

			if (tag != DW_TAG_pointer_type) {
				continue;
			}

			/*
			 * Get the address of the relocated pointer.
			 */

			void *rel_p = migrator_rtu_get_new_relocation_address
				(&pstate, rtu_data_delta, vvp->mem_p, vvp->section_p);

			if (rel_p == NULL) {
				ERROR_MSG("Could not find address for relocated variable "
					  "from @ 0x%x", (u_int32_t)vvp->mem_p);
				goto error_L2_rtu0;
			}

			/*
			 * Get the address of the variable pointed to
			 * by the relocated pointer.
			 */

		        void	*p_val	       = (void *)*(u_int32_t *)vvp->mem_p;
			void    *p_val_section = pt_get_included_section_pointer
				(&pstate, p_val);

			if (p_val_section == NULL) {
				DEBUG_MSG("Pointer not pointing to any included memory section, continuing.\n");
				continue;
			}

			void *rel_p_val = migrator_rtu_get_new_relocation_address
				(&pstate, rtu_data_delta, p_val, p_val_section);

			/*
			 * Write out the value.
			 */

			DEBUG_MSG("Setting variable @ 0x%x to 0x%x\n",
				  (u_int32_t)rel_p, (u_int32_t)rel_p_val);

			*(u_int32_t *)rel_p = (u_int32_t)rel_p_val;
		}
	}

	pt_pstate_free(&pstate);
	dwarfif_free(dbg);

	if (0) {
	error_L2_rtu0:
		pt_pstate_free(&pstate);
		dwarfif_free(dbg);
		goto error_L2;
	}

#endif /* RTU_POINTER_TRACING */

	/*
	 * Free the old software, delete old task, create new task,
	 * start it and update register_cons.
	 */

	task_register_tree *root = task_get_trc_root();

	TASK_ACQUIRE_TR_LOCK();
	RB_INSERT(task_register_tree_t, root, new_trc);
	TASK_RELEASE_TR_LOCK();

	/*
	 * BUG: the task should be free after the idle task has had
	 *      the chance to free the task_handle.
	 */
	if (!task_start(new_trc)) {
		ERROR_MSG("Could not start new task, going back to old.\n");

		TASK_ACQUIRE_TR_LOCK();
		RB_REMOVE(task_register_tree_t, root, new_trc);
		TASK_RELEASE_TR_LOCK();

		task_free(new_trc);
		SYSTEM_FREE_CALL(new_trc);
		vTaskResume(trc->task_handle);
		return 0;
	}

	TASK_ACQUIRE_TR_LOCK();
	RB_REMOVE(task_register_tree_t, root, trc);
	TASK_RELEASE_TR_LOCK();

	vTaskDelete(trc->task_handle);
	task_free(trc);
	SYSTEM_FREE_CALL(trc);
	return 1;
 error_L2:
	task_free(new_trc);
 error_L1:
	SYSTEM_FREE_CALL(new_trc);
 error_L0:
	return 0;
}
#endif /* RUNTIME_UPDATING */

void migrator_task(void *arg)
{
#ifdef RUNTIME_UPDATING
#ifdef RTUDEMO_UPDATING
	task_register_cons *trc;

	while (1) {
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
			if (!runtime_update(trc, new_sw)) {
				ERROR_MSG("Runtime updating failed.\n");
				goto error;
			}
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
			if (!runtime_update(trc, new_sw)) {
				ERROR_MSG("Runtime updating failed.\n");
				goto error;
			}
			INFO_MSG("Runtime update complete. (-> v1)\n");
			xSemaphoreGive(migrator_semaphore);
		}
	}
#endif /* RTUDEMO_UPDATING */
#ifdef RTUCONT_UPDATING
	task_register_cons *trc;

	while (1) {
#if 0
		vTaskDelay(20000/portTICK_RATE_MS);

		if ((trc = task_find("rtucont"))) {
			xSemaphoreTake(migrator_semaphore, portMAX_DELAY);
			INFO_MSG("Calling request hook.\n");
			(trc->request_hook)(cp_req_rtu);
			INFO_MSG("Returned from request hook.\n");
			xSemaphoreTake(migrator_semaphore, portMAX_DELAY);
			DEBUG_MSG("BOO! (-> v2)\n");

			Elf32_Ehdr *new_sw = (Elf32_Ehdr *)&_rtucontv2_elf_start;

			INFO_MSG("Starting runtime update.\n");
			if (!runtime_update(trc, new_sw)) {
				ERROR_MSG("Runtime updating failed.\n");
				goto error;
			}
			INFO_MSG("Runtime update complete. (-> v2)\n");
			xSemaphoreGive(migrator_semaphore);
		} else {
			goto error;
		}
#endif
		vTaskDelay(20000/portTICK_RATE_MS);

		if ((trc = task_find("rtucont"))) {
			xSemaphoreTake(migrator_semaphore, portMAX_DELAY);
			INFO_MSG("Calling request hook.\n");
			(trc->request_hook)(cp_req_rtu);
			INFO_MSG("Returned from request hook.\n");
			xSemaphoreTake(migrator_semaphore, portMAX_DELAY);
			DEBUG_MSG("BOO! (-> v2)\n");

			Elf32_Ehdr *new_sw = (Elf32_Ehdr *)&_rtucontv3_elf_start;

			INFO_MSG("Starting runtime update.\n");
			if (!runtime_update(trc, new_sw)) {
				ERROR_MSG("Runtime updating failed.\n");
				goto error;
			}
			INFO_MSG("Runtime update complete. (-> v3)\n");
			xSemaphoreGive(migrator_semaphore);
		} else {
			goto error;
		}

		vTaskDelay(20000/portTICK_RATE_MS);

		if ((trc = task_find("rtucont"))) {
			xSemaphoreTake(migrator_semaphore, portMAX_DELAY);
			INFO_MSG("Calling request hook.\n");
			(trc->request_hook)(cp_req_rtu);
			INFO_MSG("Returned from request hook.\n");
			xSemaphoreTake(migrator_semaphore, portMAX_DELAY);
			DEBUG_MSG("BOO! (-> v2)\n");

			Elf32_Ehdr *new_sw = (Elf32_Ehdr *)&_rtucontv1_elf_start;

			INFO_MSG("Starting runtime update.\n");
			if (!runtime_update(trc, new_sw)) {
				ERROR_MSG("Runtime updating failed.\n");
				goto error;
			}
			INFO_MSG("Runtime update complete. (-> v1)\n");
			xSemaphoreGive(migrator_semaphore);
		} else {
			goto error;
		}


	}
#endif /* RTUCONT_UPDATING */
error:
	ERROR_MSG("Migrator in error state, going into infinite loop.\n");
	while (1)
		;
#else /* RUNTIME_UPDATING */
	while(1) {
		vTaskDelay(1000/portTICK_RATE_MS);
	}
#endif /* RUNTIME_UPDATING */
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
