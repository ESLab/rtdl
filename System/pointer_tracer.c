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

#define SYSTEM_MODULE_NAME "POINTER_TRACER"

#include <System/pointer_tracer.h>
#include <System/tree.h>
#include <System/system.h>

#include <dwarf.h>
#include <libdwarf.h>

static int pt_internal_trace_pointer(pt_pstate *state, pt_visited_variable *v);

RB_GENERATE(pt_dyn_memsect_tree_t, pt_dyn_memsect_t, tree_e, pt_dyn_memsect_tree_cmp)
RB_GENERATE(pt_visited_variable_tree_t, pt_visited_variable_t, tree_e, pt_visited_variable_tree_cmp)

int pt_pstate_init(pt_pstate *state, Dwarf_Debug dbg, task_register_cons *trc)
{
	RB_INIT(&state->included_memsects);
	RB_INIT(&state->visited_variables);
	state->trc = trc;
	state->dbg = dbg;
	return 1;
}

int pt_trace_pointer(pt_pstate *state, Dwarf_Die type_die, void *p)
{
	pt_visited_variable *v;

	if (!dwarfif_die_has_typetag(type_die)) {
		return 0;
	}

	if ((v = pvPortMalloc(sizeof(pt_visited_variable))) == NULL) {
		ERROR_MSG("Could not allocate memory.\n");
		return 0;
	}

	v->type_die = type_die;
	v->p        = p;
	
	return pt_internal_trace_pointer(state, v);
}

static int pt_internal_trace_pointer(pt_pstate *state, pt_visited_variable *v)
{
	RB_INSERT(pt_visited_variable_tree_t, &state->visited_variables, v);

	/*
	 * Investigate if we have already visited this variable.
	 */

	void	*new_p	       = (void *)*(u_int32_t *)(v->p);
	void	*new_section_p = 0;

	pt_visited_variable search_criterion = { .p = new_p };

	pt_visited_variable *found_v =
		RB_FIND(pt_visited_variable_tree_t, 
			&state->visited_variables, &search_criterion);

	if (found_v != NULL) {
		/*
		 * We have already visited this variable, return.
		 */
		DEBUG_MSG("Variable already visited\n");
		return 1;
	}

	/*
	 * Check if we should add a dynamic section to the
	 * included_memsects tree.
	 */

	task_dynmemsect_cons *tdc = task_find_dynmemsect(state->trc, new_p);

	if (tdc != NULL) {
		/*
		 * Add this memory section to the included_memsects
		 * tree, if it is not already included.
		 */
		pt_dyn_memsect  criterion     = { .p = tdc };
		pt_dyn_memsect *found_memsect = 
			RB_FIND(pt_dyn_memsect_tree_t,
				&state->included_memsects,
				&criterion);
		new_section_p = tdc->ptr;
		
		if (found_memsect == NULL) {
			/*
			 * Include this memory section.
			 */
			pt_dyn_memsect *dms;
			if ((dms = pvPortMalloc(sizeof(pt_dyn_memsect))) == NULL) {
				ERROR_MSG("Could not allocate memory.\n");
				return 0;
			}
			dms->p = tdc;
			RB_INSERT(pt_dyn_memsect_tree_t, &state->included_memsects, dms);
		}
	} else {
		/*
		 * Is this a pointer to some non-dynamic known memory
		 * section?, e.g. the tasks continous memory.
		 */
		task_register_cons	*trc	       = state->trc;
		u_int32_t		 start_address = (u_int32_t)trc->cont_mem;
		u_int32_t		 end_address   = start_address + trc->cont_mem_size;
		u_int32_t		 address       = (u_int32_t)new_p;

		if (start_address > address || end_address <= address) {
			return 1;
		} else {
			new_section_p = trc->cont_mem;
			DEBUG_MSG("Found variable in static section.\n");
		}
	}

	/*
	 * Make a new visited_variable node.
	 */

	Dwarf_Die new_type_die = dwarfif_follow_attr_until(state->dbg, v->type_die, DW_AT_type, DW_TAG_pointer_type);
	
	if (new_type_die == NULL) {
		/*
		 * We did not find a new pointer, return.
		 */
		DEBUG_MSG("Did not find a new pointer.\n");
		return 1;
	}
	
	if (new_section_p == NULL) {
		/*
		 * Pointer pointing to something other than a memory
		 * section.
		 */
		return 1;
	}


	pt_visited_variable *new_v;

	if ((new_v = pvPortMalloc(sizeof(pt_visited_variable))) == NULL) {
		ERROR_MSG("Could not allocate memory.\n");
		return 0;
	}

	new_v->type_die	 = new_type_die;
	new_v->p	 = new_p;
	new_v->section_p = new_section_p;
	
	DEBUG_MSG("Following pointer 0x%x\n", (u_int32_t)new_p);

	return pt_internal_trace_pointer(state, new_v);
}
