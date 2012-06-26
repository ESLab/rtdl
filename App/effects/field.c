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

int main()
{
	int t;
	effect_field_state effect_state;
	u_int16_t *framebuffer1=(u_int16_t *)0x4c000000;
	//u_int16_t *framebuffer2=(u_int16_t *)0x4c400000;

	InitializeScreen640x480(RGB16BitMode,framebuffer1);

	InitializeField(&effect_state, 640, 480);

	SetScreenFrameBuffer(framebuffer1);

	int lasttime=0;

	for(t=0;;t++)
	{
		if((t&15)==0)
		{
			int time=xTaskGetTickCount();
			int fps=10*16*1000/(time-lasttime);

			printf("%d.%01d FPS\r\n",fps/10,fps%10);

			lasttime=time;
		}

		taskYIELD();

		DrawField(&effect_state, framebuffer1);
		/* SetScreenFrameBuffer(framebuffer2); */

		/* taskYIELD(); */

		/* DrawField(&effect_state, framebuffer1); */
		/* SetScreenFrameBuffer(framebuffer1); */
	}
}
