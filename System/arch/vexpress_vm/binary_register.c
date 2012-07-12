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

#define VX_BINARY_REGISTER VX_BINARY_REGISTER
#define SYSTEM_MODULE VX_BINARY_REGISTER

#include <FreeRTOS.h>

#include <string.h>

#include <System/types.h>
#include <System/system.h>
#include <System/task_manager.h>
#include <System/arch/vexpress_vm/binary_register.h>
#include <System/arch/vexpress_vm/memory_layout.h>

binary_register_entry *find_binary_register_entry(const char *name, binary_register_entry *bre)
{
	int i;
	for (i = 0; bre[i].binary_name != NULL; i++) {
		if (strcmp(name, bre[i].binary_name) == 0)
			return &bre[i];
	}
	return NULL;
}

int alloc_link_start_from_binary_register(const char *new_task_name, const char *binary_name)
{
	xMemoryInformationType	*mit	      = MIS_START_ADDRESS;
	binary_register_entry	*bin_register = (binary_register_entry *)mit[portCORE_ID()].phys_binary_register_begin;
	binary_register_entry	*bre	      = find_binary_register_entry(binary_name, bin_register);

	if (bre == NULL) {
		ERROR_MSG("Could not find binary with name \"%s\"\n", binary_name);
		goto error0;
	}

	task_register_cons *trc = task_register(new_task_name, bre->elfh);

	if (trc == NULL) {
		ERROR_MSG("Could not register task \"%s\", made from binary \"%s\"\n", new_task_name, binary_name);
		goto error0;
	}

	if (!task_alloc(trc)) {
		ERROR_MSG("Could not alloc memory for task \"%s\", made from binary \"%s\"\n", new_task_name, binary_name);
		goto error0;
	}

	if (!task_link(trc)) {
		ERROR_MSG("Could not link task \"%s\", made from binary \"%s\"\n", new_task_name, binary_name);
		goto error0;
	}

	if (!task_start(trc)) {
		ERROR_MSG("Could not start task \"%s\", made from binary \"%s\"\n", new_task_name, binary_name);
		goto error0;
	}

	return 1;
error0:
	return 0;
}
