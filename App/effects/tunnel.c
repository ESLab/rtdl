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

int main()
{
	effect_tunnel_state ets;
	u_int16_t *framebuffer1=(u_int16_t *)0x4c000000;
#ifdef TUNNEL_DBL_BUFFER
	u_int16_t *framebuffer2=(u_int16_t *)0x4c400000;
#endif /* TUNNEL_DBL_BUFFER */
	int t;

	printf("Initializing tunnel effect...\n");

	InitializeScreen640x480(RGB16BitMode,framebuffer1);

	if (!InitializeTunnel(&ets, 320, 240, 640, 480, 0,0)) {
		printf("Could not initiate tunnel effect.\n");
		while (1)
			;
	}

	int lasttime=0;

	printf("Starting rendering tunnel effect...\n");

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

			printf("tunnel: %d.%01d FPS\r\n",fps/10,fps%10);

			lasttime=time;
		}
#ifdef TUNNEL_DBL_BUFFER
		taskYIELD();

		SetScreenFrameBuffer(framebuffer1);
		DrawTunnel(&ets, framebuffer2);

		taskYIELD();

		SetScreenFrameBuffer(framebuffer2);
		DrawTunnel(&ets, framebuffer1);
#else /* TUNNEL_DBL_BUFFER */
		taskYIELD();

		DrawTunnel(&ets, framebuffer1);
#endif /* TUNNEL_DBL_BUFFER */
	}
}
