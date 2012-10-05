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

#include <App/effects/config_effect.h>

int effect_config
(task_register_cons	*trc,
 u_int16_t		 init_width,
 u_int16_t		 init_height,
 u_int16_t		 init_w_offset,
 u_int16_t		 init_h_offset)
{
	/*
	 * Set init width.
	 */

	u_int16_t *task_init_width =
		task_get_symbol_address(trc, "init_width");

	if (task_init_width == NULL) {
		return 0;
	}

	*task_init_width = init_width;

	/*
	 * Set init height.
	 */

	u_int16_t *task_init_height =
		task_get_symbol_address(trc, "init_height");

	if (task_init_height == NULL) {
		return 0;
	}

	*task_init_height = init_height;

	/*
	 * Set init width offset.
	 */

	u_int16_t *task_init_w_offset =
		task_get_symbol_address(trc, "init_w_offset");

	if (task_init_w_offset == NULL) {
		return 0;
	}

	*task_init_w_offset = init_w_offset;

	/*
	 * Set init height offset.
	 */

	u_int16_t *task_init_h_offset =
		task_get_symbol_address(trc, "init_h_offset");

	if (task_init_h_offset == NULL) {
		return 0;
	}

	*task_init_h_offset = init_h_offset;

	return 1;
}

int effect_start_and_config
(const char		*new_task_name,
 const char		*binary_name,
 binary_register_entry	*bre,
 u_int16_t		 init_width,
 u_int16_t		 init_height,
 u_int16_t		 init_w_offset,
 u_int16_t		 init_h_offset)
{
	if (!alloc_link_start_from_binary_register(new_task_name, binary_name, bre)) {
		return 0;
	}

	task_register_cons *trc = task_find(new_task_name);
	if (trc == NULL) {
		return 0;
	}

	return effect_config(trc,
			     init_width, init_height,
			     init_w_offset, init_h_offset);
}
