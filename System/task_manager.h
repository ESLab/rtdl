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

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <task.h>

#include <System/queue.h>
#include <System/tree.h>
#include <System/elf.h>
#include <System/types.h>

#include <App/rtu.h>

/*
 * Defines for task_section structure. Implemented as a double-linked
 * list.
 */

typedef struct task_section_cons_t {
	const char      *name;
	Elf32_Half       section_index;
	void		*amem;
	LIST_ENTRY(task_section_cons_t) sections;
} task_section_cons;

typedef LIST_HEAD(task_section_list_t,
		  task_section_cons_t)
     task_section_list;

/*
 * Defines for task_dynmemsect structure. Implemented as a splay tree
 * for smaller memory foot print than rb trees.
 */

typedef struct task_dynmemsect_cons_t {
	size_t	 size;
	void	*ptr;
	SPLAY_ENTRY(task_dynmemsect_cons_t) dynmemsects;
} task_dynmemsect_cons;

typedef SPLAY_HEAD(task_dynmemsect_tree_t,
		   task_dynmemsect_cons_t)
     task_dynmemsect_tree;

/*
 * Defines for task_register structure. Implemented as a rb tree for
 * fast search when doing malloc()/free().
 */

typedef struct task_register_cons_t {
	const char		*name;
	Elf32_Ehdr		*elfh;
	xTaskHandle		 task_handle;
	request_hook_fn_t	 request_hook;
	void                    *cont_mem;
	task_section_list	 sections;
	task_dynmemsect_tree	 dynmemsects;
	RB_ENTRY(task_register_cons_t) tasks;
} task_register_cons;

typedef RB_HEAD(task_register_tree_t,
		task_register_cons_t)
     task_register_tree;

/*
 * Prototypes and compare function for task_dynmemsect structure.
 */

static __inline__ int task_dynmemsect_cons_cmp
(task_dynmemsect_cons *op1, task_dynmemsect_cons *op2)
{
	return (u_int32_t)op1->ptr > (u_int32_t)op2->ptr ? -1 : 1;
}

SPLAY_PROTOTYPE(task_dynmemsect_tree_t, task_dynmemsect_cons_t, dynmemsects, task_dynmemsect_cons_cmp)


/*
 * Prototypes and compare function for task_register structure.
 */

static __inline__ int task_register_cons_cmp
(task_register_cons *op1, task_register_cons *op2)
{
	return (u_int32_t)op1->task_handle > (u_int32_t)op2->task_handle ? -1 : 1;
}

RB_PROTOTYPE(task_register_tree_t, task_register_cons_t, tasks, task_register_cons_cmp)

/*
 * Prototypes.
 */

task_register_tree	*task_get_trc_root();
task_register_cons	*task_find(const char *name);
void			*task_get_section_address(task_register_cons *, Elf32_Half);
int			 task_link(task_register_cons *trc);
int			 task_alloc(task_register_cons *trc);
int			 task_free(task_register_cons *trc);
int			 task_start(task_register_cons *trc);
task_register_cons	*task_register(const char *name, Elf32_Ehdr *elfh);

#endif /* TASK_MANAGER_H */
