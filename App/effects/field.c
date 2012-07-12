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

#include <App/effects/field_effect.h>
#include <App/effects/Utils.h>
#include <App/rtu.h>

int tm_requested = 0;

#if 0
static void write_fb_8bit_to_16bit(u_int8_t *fb8bit, u_int16_t *fb16bit)
{
	int i;
	for (i = 0; i < 640*480 / 2; i++) {
		u_int32_t	*fb32 = (u_int32_t *)fb16bit;
		u_int8_t	 r1   = fb8bit[2*i  ] & 0x1f;
		u_int8_t	 g1   = fb8bit[2*i  ] & 0x1f;
		u_int8_t	 b1   = fb8bit[2*i  ] & 0x1f;
		u_int8_t	 r2   = fb8bit[2*i+1] & 0x1f;
		u_int8_t	 g2   = fb8bit[2*i+1] & 0x1f;
		u_int8_t	 b2   = fb8bit[2*i+1] & 0x1f;
		u_int16_t	 p1   = ((r1 << 11) | (g1 << 5) | (b1 << 0)) & 0xffff;
		u_int16_t	 p2   = ((r2 << 11) | (g2 << 5) | (b2 << 0)) & 0xffff;
		fb32[i]		      = (p1 << 16) | p2;
	}
}
#endif

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
	effect_field_state	 effect_state;
	u_int16_t		*framebuffer1 =	(u_int16_t *)0x4c000000;
#ifdef FIELD_DBL_BUFFER
	//u_int16_t		*framebuffer2 =	(u_int16_t *)0x4c400000;
#endif /* FIELD_DBL_BUFFER */

	InitializeScreen640x480(RGB16BitMode,framebuffer1);

	if (init_width    == 0xffff ||
	    init_height   == 0xffff ||
	    init_w_offset == 0xffff ||
	    init_h_offset == 0xffff) {
		printf("field: Config parameters not setup.\n");
		goto error;
	}

	if (!InitializeField(&effect_state,
			     init_width, init_height,
			     640, 480,
			     init_w_offset, init_h_offset)) {
		printf("Could not initiate field effect.\n");
		goto error;
	}

	SetScreenFrameBuffer(framebuffer1);

	int lasttime=0;

#ifdef FIELD_DBL_BUFFER
	for(t = 0; ; t += 2) {
#else /* FIELD_DBL_BUFFER */
	for(t = 0; ; t++) {
#endif /* FIELD_DBL_BUFFER */

		if (tm_requested) {
			TASK_IN_SAFE_STATE();
			tm_requested = 0;
		}

		if((t&15)==0) {

			int	time = xTaskGetTickCount();
			int	fps  = 10*16*1000/(time-lasttime);

			printf("field: %d.%01d FPS\r\n",fps/10,fps%10);

			lasttime = time;
		}
#ifdef FIELD_DBL_BUFFER
		taskYIELD();

		DrawField(&effect_state, framebuffer2);
		SetScreenFrameBuffer(framebuffer2);

		taskYIELD();

		DrawField(&effect_state, framebuffer1);
		SetScreenFrameBuffer(framebuffer1);
#else /* FIELD_DBL_BUFFER */
		taskYIELD();

		DrawField(&effect_state, framebuffer1);
#endif /* FIELD_DBL_BUFFER */
	}

error:
	while (1) {
		printf("Field effect in error state.\n");
		vTaskSuspend(NULL);
	}

}
