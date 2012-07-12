/***********************************************************************************/
/* Copyright (c) 2012, Dag Ågren. All rights reserved.				   */
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
/* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.			   */
/***********************************************************************************/

#include <FreeRTOS.h>

#include <task.h>

#include <stdio.h>

#include <System/types.h>
#include <System/pl111.h>

#include <App/effects/tunnel_effect.h>
#include <App/effects/Utils.h>
#include <App/rtu.h>

int tm_requested = 0;

void cpRequestHook(cp_req_t req_type)
{
	if (req_type == cp_req_tm) {
		tm_requested = 1;
	}
}

u_int16_t	init_width    = 0xffff;
u_int16_t	init_height   = 0xffff;
u_int16_t	init_w_offset = 0xffff;
u_int16_t	init_h_offset = 0xffff;

int main()
{
	int			 t;
	effect_tunnel_state	 ets;
	u_int16_t		*framebuffer1 =	(u_int16_t *)0x4c000000;
#ifdef TUNNEL_DBL_BUFFER
	u_int16_t		*framebuffer2 =	(u_int16_t *)0x4c400000;
#endif /* TUNNEL_DBL_BUFFER */
	task_register_cons	*trc;
	const char		*task_name;
	portTickType		 last_wake;
	const portTickType	 delay	      = 1000 * portTICK_RATE_MS / 25;


	trc = task_get_current_trc();

	if (trc == NULL) {
		printf("tunnel effect: could not find trc.\n");
		goto error;
	}

	task_name = trc->name;

	printf("%s: Initializing tunnel effect...\n", task_name);

	InitializeScreen640x480(RGB16BitMode,framebuffer1);

	if (init_width    == 0xffff ||
	    init_height   == 0xffff ||
	    init_w_offset == 0xffff ||
	    init_h_offset == 0xffff) {
		printf("%s: Config parameters not setup.\n", task_name);
		goto error;
	}

	if (!InitializeTunnel(&ets,
			      init_width, init_height,
			      640, 480,
			      init_w_offset, init_h_offset)) {
		printf("%s: Could not initiate tunnel effect.\n", task_name);
		goto error;
	}

	int lasttime=0;

	printf("%s: Starting rendering tunnel effect...\n", task_name);

	last_wake = xTaskGetTickCount();

#ifdef TUNNEL_DBL_BUFFER
	for(t=0;;t+=2) {
#else /* TUNNEL_DBL_BUFFER */
	for(t=0;;t++) {
#endif /* TUNNEL_DBL_BUFFER */

		if (tm_requested) {
			TASK_IN_SAFE_STATE();
			tm_requested = 0;
		}

		if((t&15)==0)
		{
			int time=xTaskGetTickCount();
			int fps=10*16*1000/(time-lasttime);

			printf("%s: %d.%01d FPS\r\n",task_name,fps/10,fps%10);

			lasttime=time;
		}
#ifdef TUNNEL_DBL_BUFFER
		taskYIELD();

		SetScreenFrameBuffer(framebuffer1);
		DrawTunnel(&ets, framebuffer2);
		vTaskDelayUntil(&last_wake, delay);

		SetScreenFrameBuffer(framebuffer2);
		DrawTunnel(&ets, framebuffer1);
#else /* TUNNEL_DBL_BUFFER */
		DrawTunnel(&ets, framebuffer1);
#endif /* TUNNEL_DBL_BUFFER */
		vTaskDelayUntil(&last_wake, delay);
	}

error:
	while (1) {
		printf("Tunnel effect in error state.\n");
		vTaskSuspend(NULL);
	}
}
