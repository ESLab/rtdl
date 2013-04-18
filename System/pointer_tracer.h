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

#ifndef POINTER_TRACER_H
#define POINTER_TRACER_H

#include <System/queue.h>
#include <System/tree.h>
#include <System/task_manager.h>
#include <System/dwarfif.h>

#include <libdwarf.h>

/*
 * A dynamic memory section that is included in the pointer graph.
 */

typedef struct pt_dyn_memsect_t {
	task_dynmemsect_cons    *p;
	RB_ENTRY(pt_dyn_memsect_t) tree_e;
} pt_dyn_memsect;

typedef RB_HEAD(pt_dyn_memsect_tree_t, pt_dyn_memsect_t) pt_dyn_memsect_tree;

static inline int pt_dyn_memsect_tree_cmp(pt_dyn_memsect *op1, pt_dyn_memsect *op2)
{
	return op1->p - op2->p;
}

RB_PROTOTYPE(pt_dyn_memsect_tree_t, pt_dyn_memsect_t, tree_e, pt_dyn_memsect_tree_cmp)

/*
 * A tree of visited variables.
 */

typedef struct pt_visited_variable_t {
	Dwarf_Die	 type_die;
	void		*p;
	void		*section_p;
	RB_ENTRY(pt_visited_variable_t) tree_e;
} pt_visited_variable;

typedef RB_HEAD(pt_visited_variable_tree_t, pt_visited_variable_t) pt_visited_variable_tree;

static inline int pt_visited_variable_tree_cmp(pt_visited_variable *op1, pt_visited_variable *op2)
{
	return op1->p - op2->p;
}

RB_PROTOTYPE(pt_visited_variable_tree_t, pt_visited_variable_t, tree_e, pt_visited_variable_tree_cmp)

/*
 * The state of a pointer trace process.
 */

typedef struct pt_pstate_t {
	pt_dyn_memsect_tree		 included_memsects;
	pt_visited_variable_tree	 visited_variables;
	task_register_cons		*trc;
	Dwarf_Debug                      dbg;
} pt_pstate;

/*
 * Function prototypes
 */

int	pt_pstate_init(pt_pstate *state, Dwarf_Debug dbg, task_register_cons *trc);
int	pt_trace_pointer(pt_pstate *state, Dwarf_Die type_die, void *p);

#endif /* POINTER_TRACER_H */
