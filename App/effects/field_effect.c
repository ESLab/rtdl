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

#include <App/effects/field_effect.h>
#include <App/effects/Utils.h>
#include <System/pl111.h>
#include <System/task_manager.h>

#include <string.h>
#include <stdio.h>
#include <math.h>

void InitializeField(effect_field_state *state, u_int16_t width, u_int16_t height)
{
	int		x,y;
	u_int16_t	half_width  = width / 2;
	u_int16_t	half_height = height / 2;
	state->width		    = width;
	state->height		    = height;
	state->rayarray		    =
		apptask_malloc(half_width * half_height * 3 * sizeof(*state->rayarray));
	state->t		    = 0;


	for(y=0; y < half_height; y++) {
		for(x=0; x < half_width; x++)
		{
			int32_t fx			      = Fix(2*x+1-half_width)/(half_width);
			int32_t fy			      = Fix(2*y+1-half_height)/(half_width);
			int32_t fz			      = -Fix(1)+isq(fx)/2+isq(fy)/2;
			int32_t r2			      = isq(fx)+isq(fy)+isq(fz);
			int32_t r			      = sqrti(r2<<12);
			state->rayarray[3*(x+y*half_width)+0] = idiv(fx,r)<<3;
			state->rayarray[3*(x+y*half_width)+1] = idiv(fy,r)<<3;
			state->rayarray[3*(x+y*half_width)+2] = idiv(fz,r)<<3;
		}
	}
}

static inline int32_t approxabs(int32_t x)
{
	return x ^ (x >> 31);
}

static inline u_int16_t get_16bit_from_8bit(u_int8_t p)
{
	u_int8_t	r   = p & 0x1f;
	u_int8_t	g   = p & 0x1f;
	u_int8_t	b   = p & 0x1f;
	u_int16_t	ret = ((r << 11) | (g << 5) | (b << 0)) & 0xffff;
	return ret;
}

void DrawField(effect_field_state *state, uint16_t *pixels)
{
	int		 x,y;
	int16_t		*rays	     = state->rayarray;
	u_int32_t	 t	     = state->t;
	u_int16_t	 half_width  = state->width / 2;
	u_int16_t	 half_height = state->height / 2;
	u_int32_t	 vi	     = 0;

	switch(t&3)
	{
	case 1:
		vi += state->width+1;
		break;
	case 2:
		vi += 1;
		break;
	case 3:
		vi += state->width;
		break;
	}

	int32_t x0    = (0 + Fix(0.5)) << 20;
	int32_t y0    = (isin(t * 20) + Fix(0.5)) << 20;
	int32_t z0    = (t * 150 + Fix(0.5)) << 20;
	int32_t sin_a =	isin(t * 9);
	int32_t cos_a =	icos(t * 9);

	for(y=0;y<half_height;y++)
	{
		for(x=0;x<half_width;x++)
		{
			int32_t	dx = *rays++;
			int32_t	dy = *rays++;
			int32_t	dz = *rays++;
			int32_t	t  = (dx*cos_a+dz*sin_a)>>12;
			        dz = (-dx*sin_a+dz*cos_a)>>12;
				dx = t;
			int32_t	x  = x0;
			int32_t y  = y0;
			int32_t z  = z0;
			int	i  = 31;

			while(i)
			{
#if 0
				int32_t tx = approxabs(x)>>15;
				int32_t ty = approxabs(y)>>15;
				int32_t tz = approxabs(z)>>15;

				int32_t dist	 = tx;
				if(ty>dist) dist = ty;
				if(tz>dist) dist = tz;

				dist -=	0x8000;

				if(dist<0x800) break;
#endif

#if 0
				int32_t tx=approxabs(x)>>16;
				int32_t ty=approxabs(y)>>16;
				int32_t tz=approxabs(z)>>16;

				int32_t dist=tx+ty+tz-0x8000;

				if(dist<0x200) break;
#endif

#if 0
				int32_t tx=approxabs(x)>>15;
				int32_t ty=approxabs(y)>>15;
				int32_t tz=approxabs(z)>>15;

				int32_t dist1=tx;
				if(tz>dist1) dist1=tz;

				int32_t dist2=tx;
				if(ty>dist2) dist2=ty;

				int32_t dist=dist1;
				if(dist2<dist1) dist=dist2;

				dist-=0x4000;

				if(dist<0x400) break;
#endif

#if 0
				int32_t tx=approxabs(x)>>15;
				int32_t ty=approxabs(y)>>15;
				int32_t tz=approxabs(z)>>15;

				int32_t dist1=tx;
				if(tz>dist1) dist1=tz;

				int32_t dist2=tx;
				if(ty>dist2) dist2=ty;

				int32_t dist3=ty;
				if(tz>dist3) dist3=tz;

				int32_t dist=dist1;
				if(dist2<dist) dist=dist2;
				if(dist3<dist) dist=dist3;

				dist-=0x4000;

				if(dist<0x400) break;
#endif

				int32_t tx   = approxabs(x) >> 16;
				int32_t ty   = approxabs(y) >> 16;
				int32_t tz   = approxabs(z) >> 16;
//				int32_t dist = approxabs(ty - tz) + tx - 0x3000;
				int32_t dist = (tx | (ty + tz)) - 0x5000;

				if(dist<0x200)
					break;

				x+=dx*dist;
				y+=dy*dist;
				z+=dz*dist;

				i--;
			}

			pixels[vi]=get_16bit_from_8bit(i);
			vi += 2;
		}
		vi += state->width;
	}
	state->t++;
}
