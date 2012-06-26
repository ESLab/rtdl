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

#include <App/effects/tunnel_effect.h>
#include <App/effects/Utils.h>
#include <System/pl111.h>
#include <System/task_manager.h>

#include <string.h>
#include <stdio.h>
#include <math.h>

//#define PI 3.1415926535897932384626433832795

/* static uint32_t lookup32[Width*Height/2]; */
/* static uint16_t texture[256*256]; */

static uint16_t RGB(int r,int g,int b)
{
	if(r<0) r=0; if(r>255) r=255;
	if(g<0) g=0; if(g>255) g=255;
	if(b<0) b=0; if(b>255) b=255;

	r>>=3;
	g>>=2;
	b>>=3;

	return (r<<11)|(g<<5)|b;
}

int InitializeTunnel
(effect_tunnel_state	*state,
 uint16_t		 width,
 uint16_t		 height,
 u_int16_t		 phys_width,
 u_int16_t		 phys_height,
 u_int16_t		 w_offset,
 u_int16_t		 h_offset)
{
	int		 x,y;
       	state->width		= width;
	state->height		= height;
	state->phys_width	= phys_width;
	state->phys_height	= phys_height;
	state->w_offset		= w_offset;
	state->h_offset		= h_offset;
	state->t		= 0;
	state->lookup32		= apptask_malloc(state->width * state->height * sizeof(uint32_t) / 2);
	uint16_t	*lookup = (uint16_t *)state->lookup32;
	state->texture		= apptask_malloc(256*256*sizeof(uint16_t));

	if (state->lookup32 == NULL || state->texture == NULL) {
		printf("Malloc failed!\n");
		return 0;
	}

	for (y = 0; y < state->height; y++) {
		for (x = 0; x < state->width; x++) {
			float	fx		 = (float)(2*x+1-state->width)/(2*state->width);
			float	fy		 = (float)(2*y+1-state->height)/(2*state->width);
			float	r		 = sqrtf(fx*fx+fy*fy);
			float	a		 = atan2f(fy,fx);
			float	z		 = 0.3*sinf(a*3)/r;
			int	tu		 = 256*(a/(2*M_PI)+z/6);
			int	tv		 = 256*z;
			lookup[x+y*state->width] = ((tv&0xff)<<8)|(tu&0xff);
		}
	}

	for (y = 0; y < 256; y++) {
		for (x = 0; x < 256; x++) {
			int32_t bright		= (isin(x*16)+Fix(1))/3;
			state->texture[x+y*256] = RGB(
				imul((isin(y*16)-1)/4,bright),
				imul((isin(y*16+4096/3)-1)/4,bright),
				imul((isin(y*16+2*4096/3)-1)/4,bright)
				);
		}
	}
	return 1;
}

void DrawTunnel
(effect_tunnel_state	*state,
 uint16_t		*pixels)
{
	int		 i;
	uint32_t	*pixels32 = (uint32_t *)pixels;
	uint32_t	 t	  = state->t * 4;
	int		 du	  = isin(t)>>2;
	int		 dv	  = 2*t;
	int		 d	  = (dv<<8)+du;

	for(i=0;i<state->width*state->height/2;i++)
	{
		uint32_t	val		= state->lookup32[i];
		uint32_t	pixel1		= state->texture[((val>>16)+d)&0xffff];
		uint32_t	pixel2		= state->texture[((val&0xffff)+d)&0xffff];
		u_int16_t	phys_col	= (2*i) % state->width + state->w_offset;
		u_int16_t	phys_row	= (2*i) / state->width + state->h_offset;
		u_int32_t	phys_row_offset = phys_row * state->phys_width;

		pixels32[(phys_row_offset + phys_col)/2] = (pixel1<<16)|pixel2;

		//pixels32[i]	       = (pixel1<<16)|pixel2;
	}
	state->t++;
}


#if 0
void DrawTunnel(uint16_t *pixels,int t)
{
	uint32_t *pixels32=(uint32_t *)pixels;

	t*=4;
	int du=isin(t)>>2;
	int dv=2*t;
	int d=(dv<<8)+du;

	for(int i=0;i<Width*Height/2;i++)
	{
		uint32_t val=lookup32[i];
		uint32_t pixel1=texture[((val>>16)+d)&0xffff];
		uint32_t pixel2=texture[((val&0xffff)+d)&0xffff];

		pixels32[i]=(pixel1<<16)|pixel2;
	}
}
#endif




