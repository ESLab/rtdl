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
#include <System/system_util.h>

#include <dwarf.h>
#include <libdwarf.h>

static int pt_internal_struct_iterator_cb(pt_pstate *pstate, Dwarf_Die die, void *arg);
static int pt_internal_trace_pointer(pt_pstate *state, pt_visited_variable *v);
static int pt_internal_trace_var(pt_pstate *state, pt_visited_variable *v);
static int pt_internal_trace_type(pt_pstate *state, pt_visited_variable *v);
static int pt_iterate_die_and_siblings(pt_pstate *pstate, Dwarf_Die die, pt_die_cb_fun_t *fun, void *arg);

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

int pt_pstate_free(pt_pstate *pstate)
{
	{
		pt_dyn_memsect *p, *tp;
		RB_FOREACH_SAFE(p, pt_dyn_memsect_tree_t, &pstate->included_memsects, tp) {
			RB_REMOVE(pt_dyn_memsect_tree_t, &pstate->included_memsects, p);
			SYSTEM_FREE_CALL(p);
		}
	}

	{
		pt_visited_variable *p, *tp;
		RB_FOREACH_SAFE(p, pt_visited_variable_tree_t, &pstate->visited_variables, tp) {
			RB_REMOVE(pt_visited_variable_tree_t, &pstate->visited_variables, p);
			SYSTEM_FREE_CALL(p);
		}
	}
	return 1;
}

int pt_trace_pointer(pt_pstate *state, Dwarf_Die type_die, void *p)
{
	pt_visited_variable *v;

	if (!dwarfif_die_has_typetag(type_die)) {
		ERROR_MSG("%s: Die has not type tag.\n", __func__);
		return 0;
	}

	if ((v = SYSTEM_MALLOC_CALL(sizeof(pt_visited_variable))) == NULL) {
		ERROR_MSG("Could not allocate memory.\n");
		return 0;
	}

	v->type_die  = type_die;
	v->mem_p     = p;
	v->section_p = NULL;

	return pt_internal_trace_type(state, v);
}


static int pt_internal_struct_iterator_cb(pt_pstate *pstate, Dwarf_Die die, void *arg)
{
	pt_visited_variable	*v = (pt_visited_variable *)arg;
	Dwarf_Error		 err;
	Dwarf_Half		 tag;
	Dwarf_Attribute	 loc_attr;
	Dwarf_Unsigned		 loc;

	if (dwarf_tag(die, &tag, &err) != DW_DLV_OK) {
		ERROR_MSG("%s: Could not get tag from die.\n", __func__);
		return 0;
	}

	if (tag != DW_TAG_member) {
		if (tag == DW_TAG_structure_type) {
			return 1;
		} else {
			const char *s;
			if (dwarf_get_TAG_name(tag, &s) != DW_DLV_OK) {
				ERROR_MSG("%s: Could not get tag name.\n", __func__);
				return 0;
			}
			ERROR_MSG("%s: %s is non-member tag in structure.\n", __func__, s);
			return 0;
		}
	}

	if (dwarf_attr(die, DW_AT_data_member_location, &loc_attr, &err) != DW_DLV_OK) {
		return 0;
	}

	if (dwarf_formudata(loc_attr, &loc, &err) != DW_DLV_OK) {
		return 0;
	}

	pt_visited_variable *new_v;

	if ((new_v = SYSTEM_MALLOC_CALL(sizeof(pt_visited_variable))) == NULL) {
		ERROR_MSG("%s: Could not allocate memory.\n", __func__);
		return 0;
	}

	new_v->type_die  = die;
	new_v->mem_p	 = (void *)((npi_t)v->mem_p + (npi_t)loc);
	new_v->section_p = v->section_p;
	DEBUG_MSG("%s: following variable @ 0x%x\n", __func__, (npi_t)new_v->mem_p);

	if (!pt_internal_trace_type(pstate, new_v)) {
		SYSTEM_FREE_CALL(new_v);
		return 0;
	} else {
		return 1;
	}
}

static int pt_internal_trace_type(pt_pstate *state, pt_visited_variable *v)
{
	do {
		Dwarf_Error	err;
		Dwarf_Half	tag;

		if (dwarf_tag(v->type_die, &tag, &err) != DW_DLV_OK) {
			ERROR_MSG("%s: Could not get tag from die.\n", __func__);
			return 0;
		}

		{
			const char *s;
			if (dwarf_get_TAG_name(tag, &s) != DW_DLV_OK) {
				ERROR_MSG("%s: Could not get tag name.\n", __func__);
				return 0;
			}

			DEBUG_MSG("%s: found tag %s\n", __func__, s);
		}

		switch (tag) {
		case DW_TAG_pointer_type:
			DEBUG_MSG("%s: Adding pointer @ 0x%x\n", __func__, (npi_t)v->mem_p);
			return pt_internal_trace_pointer(state, v);
		case DW_TAG_structure_type:
			DEBUG_MSG("%s: Iterating through structure.\n", __func__);

			Dwarf_Die child_die;

			if (dwarf_child(v->type_die, &child_die, &err) != DW_DLV_OK) {
				ERROR_MSG("%s: Could not find children to structure_type tag.\n", __func__);
				return 0;
			}

			return pt_iterate_die_and_siblings(state, child_die,
							   pt_internal_struct_iterator_cb, v);
		case DW_TAG_base_type:
			DEBUG_MSG("%s: Adding base var @ 0x%x\n", __func__, (npi_t)v->mem_p);
			return pt_internal_trace_var(state, v);
		}
	} while ((v->type_die = dwarfif_follow_attr(state->dbg, v->type_die, DW_AT_type)) != NULL);
	return 1;
}

static int pt_internal_trace_var(pt_pstate *state, pt_visited_variable *v)
{
	RB_INSERT(pt_visited_variable_tree_t, &state->visited_variables, v);

	return 1;
}

static int pt_internal_trace_pointer(pt_pstate *state, pt_visited_variable *v)
{
	if (!pt_internal_trace_var(state, v)) {
		ERROR_MSG("%s: failed to add pointer as var first.\n", __func__);
		return 0;
	}

	{
		/*
		 * Sanity check.
		 */
		Dwarf_Error	err;
		Dwarf_Half	tag;

		if (dwarf_tag(v->type_die, &tag, &err) != DW_DLV_OK) {
			ERROR_MSG("pt_internal_trace_pointer(): Could not get tag for type.\n");
			return 0;
		}

		if (tag != DW_TAG_pointer_type) {
			ERROR_MSG("Trying to trace a non-pointer variable.\n");
			return 0;
		}
	}

	/*
	 * Investigate if we have already visited this variable.
	 */

	void	*new_p	       = (void *)*(u_int32_t *)(v->mem_p);
	void	*new_section_p = 0;

	pt_visited_variable search_criterion = { .mem_p = new_p };

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
		pt_dyn_memsect  criterion     = { .tdc_p = tdc };
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
			if ((dms = SYSTEM_MALLOC_CALL(sizeof(pt_dyn_memsect))) == NULL) {
				ERROR_MSG("Could not allocate memory.\n");
				return 0;
			}
			dms->tdc_p   = tdc;
			dms->delta_p = 0;
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

	Dwarf_Die	next_type_die = dwarfif_follow_attr(state->dbg, v->type_die, DW_AT_type);

	if (next_type_die == NULL) {
		/*
		 * No new type die, we can't continue.
		 */
		DEBUG_MSG("Did not find any new type.\n");
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

	if ((new_v = SYSTEM_MALLOC_CALL(sizeof(pt_visited_variable))) == NULL) {
		ERROR_MSG("Could not allocate memory.\n");
		return 0;
	}

	new_v->type_die  = next_type_die;
	new_v->mem_p	 = (void *)new_p;
	new_v->section_p = new_section_p;
	DEBUG_MSG("%s: Following variable @ 0x%x\n", __func__, (npi_t)new_p);

	if (!pt_internal_trace_type(state, new_v)) {
		SYSTEM_FREE_CALL(new_v);
		return 0;
	} else {
		return 1;
	}
}

static int pt_iterate_die_1(pt_pstate *pstate, Dwarf_Die die, pt_die_cb_fun_t *fun, void *arg)
{
	return (*fun)(pstate, die, arg);
}

static int pt_iterate_die_and_siblings(pt_pstate *pstate, Dwarf_Die die, pt_die_cb_fun_t *fun, void *arg)
{
	Dwarf_Die	cur_die = die;
	Dwarf_Die	child;
	Dwarf_Error	err;
	int		res;

	if (!pt_iterate_die_1(pstate, cur_die, fun, arg)) {
		ERROR_MSG("%s: Failed iterate over die itself.\n", __func__);
		return 0;
	}

	while (1) {
		Dwarf_Die sib_die;
		if ((res = dwarf_child(cur_die, &child, &err)) == DW_DLV_ERROR) {
			ERROR_MSG("%s: Could not get child.\n", __func__);
			return 0;
		}
		if (res == DW_DLV_OK) {
			/*
			 * We found a child.
			 */
			if (!pt_iterate_die_and_siblings(pstate, child, fun, arg)) {
				return 0;
			}
		}
		/*
		 * Continue with the siblings.
		 */
		if ((res = dwarf_siblingof(pstate->dbg, cur_die, &sib_die, &err)) == DW_DLV_ERROR) {
			ERROR_MSG("%s: Could not get sibling.\n", __func__);
			return 0;
		}
		if (res == DW_DLV_NO_ENTRY) {
			return 1;
		}
		if (cur_die != die) {
			dwarf_dealloc(pstate->dbg, cur_die, DW_DLA_DIE);
		}
		cur_die = sib_die;
		if (!pt_iterate_die_1(pstate, cur_die, fun, arg)) {
			ERROR_MSG("%s: pt_iterate_die_1() failed.\n", __func__);
			return 0;
		}
	}
}

int pt_iterate_dies(pt_pstate *pstate, pt_die_cb_fun_t *fun, void *arg)
{
    Dwarf_Unsigned	cu_header_length = 0;
    Dwarf_Half		version_stamp	 = 0;
    Dwarf_Unsigned	abbrev_offset	 = 0;
    Dwarf_Half		address_size	 = 0;
    Dwarf_Half		length_size	 = 0;
    Dwarf_Half		extension_size	 = 0;
    Dwarf_Unsigned	next_cu_header	 = 0;
    Dwarf_Error		err;
    int			cu_number;
    int			res;

    /*
     * Iterate through the compilation units.
     */

    for (cu_number = 0; ; cu_number++) {
	    Dwarf_Die no_die = 0;
	    Dwarf_Die cu_die;

	    if ((res = dwarf_next_cu_header_b(pstate->dbg, &cu_header_length,
					      &version_stamp, &abbrev_offset, &address_size,
					      &length_size, &extension_size, &next_cu_header,
					      &err)) == DW_DLV_ERROR) {
		    ERROR_MSG("%s: Error in dwarg_next_cu_header (%s)\n", __func__, dwarf_errmsg(err));
		    return 0;
	    }
	    if (res == DW_DLV_NO_ENTRY) {
		    return 1;
	    }

	    if (dwarf_siblingof(pstate->dbg, no_die, &cu_die, &err) != DW_DLV_OK) {
		    ERROR_MSG("%s: Error in dwarf_siblingof (%s)\n", __func__, dwarf_errmsg(err));
	    }

	    if (!pt_iterate_die_and_siblings(pstate, cu_die, fun, arg)) {
		    ERROR_MSG("%s: Error iterating dies.\n", __func__);
		    return 0;
	    }

	    dwarf_dealloc(pstate->dbg, cu_die, DW_DLA_DIE);
    }
}

void *pt_get_included_section_pointer(pt_pstate *pstate, void *p)
{
	task_dynmemsect_cons	criterion_a = { .ptr   = p };
	pt_dyn_memsect		criterion_b = { .tdc_p = &criterion_a };

	pt_dyn_memsect *dms = RB_NFIND(pt_dyn_memsect_tree_t, &pstate->included_memsects, &criterion_b);

	void	*section_p;
	size_t	 section_size;

	if (dms == NULL) {
		/*
		 * Did not find a dynamic section. We will fall back
		 * on the static section.
		 */
		section_p    = pstate->trc->cont_mem;
		section_size = pstate->trc->cont_mem_size;
	} else {
		/*
		 * We did find a dynamic section, return it if the pointer
		 * appears to be in it.
		 */
		section_p    = dms->tdc_p->ptr;
		section_size = dms->tdc_p->size;
	}

	return util_pointer_in_section
		(section_p, section_size, p) ?
		section_p : NULL;
}
