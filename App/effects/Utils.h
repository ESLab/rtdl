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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef FloatToFixed
#undef FloatToFixed
#undef IntToFixed
#undef FixedToFloat
#undef FixedToInt
#endif


//
// 20.12 fixed-point operations
//

#define Fix(val) ((int32_t)((val)*4096))
#define F(val) Fix(val)

#define FixSqrt2 5793
#define FixInvSqrt2 2896
#define FixSinPiDiv4 FixInvSqrt2
#define FixCosPiDiv4 FixInvSqrt2
#define FixSinPiDiv8 1567
#define FixCosPiDiv8 3784

static inline int32_t FloatToFixed(float val) { return val*4096.0f; }
static inline int32_t IntToFixed(int val) { return val<<12; }
static inline int32_t IntToFixedPlusHalf(int val) { return (val<<12)+0x800; }

static inline float FixedToFloat(int32_t val) { return (float)val/4096.0; }
static inline int32_t FixedToInt(int32_t val) { return val>>12; }
static inline int32_t FixedToRoundedInt(int32_t val) { return (val+0x800)>>12; }

//static inline int32_t FixedMul(int32_t a,int32_t b) { return ((int64_t)a*(int64_t)b)>>16; }

static inline int32_t imin(int32_t a,int32_t b) { return a<b?a:b; }
static inline int32_t imax(int32_t a,int32_t b) { return a>b?a:b; }
static inline int32_t iabs(int32_t a) { return a<0?-a:a; }
static inline int32_t isign(int32_t a) { return a>0?1:a<0?-1:0; }

static inline int64_t imul64(int64_t a, int64_t b) { return (a*b)>>12; }
static inline int32_t imul(int32_t a, int32_t b) { return (int32_t)imul64(a,b); }

static inline int64_t idiv64(int64_t num,int64_t den) { return (num<<12)/den; }
static inline int32_t idiv(int32_t num,int32_t den) { return (int32_t)idiv64(num,den); }

static inline int64_t isq64(int64_t val) { return imul64(val,val); }
static inline int32_t isq(int32_t val) { return imul(val,val); }

static uint32_t sqrti(uint32_t n)
{
	uint32_t s,t;

	#define sqrtBit(k) \
	t = s+(1UL<<(k-1)); t <<= k+1; if (n >= t) { n -= t; s |= 1UL<<k; }

	s=0;
	if(n>=1<<30) { n-=1<<30; s=1<<15; }
	sqrtBit(14); sqrtBit(13); sqrtBit(12); sqrtBit(11); sqrtBit(10);
	sqrtBit(9); sqrtBit(8); sqrtBit(7); sqrtBit(6); sqrtBit(5);
	sqrtBit(4); sqrtBit(3); sqrtBit(2); sqrtBit(1);
	if(n>s<<1) s|=1;

	#undef sqrtBit

	return s;
}

static inline int64_t isqrt64(int64_t val) { return sqrti((uint64_t)val<<12); }
static inline int32_t isqrt(int32_t val) { return sqrti((uint64_t)val<<12); }

static inline int64_t inorm64(int64_t a,int64_t b) { return sqrt((double)a*(double)a+(double)b*(double)b); }
static inline int32_t inorm(int32_t a,int32_t b) { return sqrt((double)a*(double)a+(double)b*(double)b); }

static inline int32_t ifloor(int a) { return a&~0xfff; }
static inline int32_t ifrac(int a) { return a&0xfff; }

int32_t isin(int a);
static inline int32_t icos(int a) { return isin(a+1024); }
static inline int32_t itan(int a) { return idiv(isin(a),icos(a)); } // TODO: use a table
static inline int32_t idegrees(int degrees) { return degrees*4096.0f/360.0f; }
static inline int32_t iradians(float radians) { return radians*4096.0f/2/3.1415926535897932; }

static inline int32_t ilerp(int32_t a,int32_t b,int32_t t)
{
	return a+imul(b-a,t);
}

/*static inline int32_t islerp(int32_t a,int32_t b,int32_t t)
{
	return a;
}*/

//
// Bitwise operations
//

static inline int CountBits32(uint32_t val)
{
	val=(val&0x55555555)+((val&0xaaaaaaaa)>>1);
	val=(val&0x33333333)+((val&0xcccccccc)>>2);
	val=(val&0x0f0f0f0f)+((val&0xf0f0f0f0)>>4);
	val=(val&0x00ff00ff)+((val&0xff00ff00)>>8);
	val=(val&0x0000ffff)+((val&0xffff0000)>>16);
	return val;
}

static inline int CountBits16(uint16_t val)
{
	val=(val&0x5555)+((val&0xaaaa)>>1);
	val=(val&0x3333)+((val&0xcccc)>>2);
	val=(val&0x0f0f)+((val&0xf0f0)>>4);
	val=(val&0x00ff)+((val&0xff00)>>8);
	return val;
}

static inline int CountBits8(uint8_t val)
{
	val=(val&0x55)+((val&0xaa)>>1);
	val=(val&0x33)+((val&0xcc)>>2);
	val=(val&0x0f)+((val&0xf0)>>4);
	return val;
}

static inline uint32_t ReverseBits32(uint32_t val)
{
	val=((val>>1)&0x55555555)|((val<<1)&0xaaaaaaaa);
	val=((val>>2)&0x33333333)|((val<<2)&0xcccccccc);
	val=((val>>4)&0x0f0f0f0f)|((val<<4)&0xf0f0f0f0);
	val=((val>>8)&0x00ff00ff)|((val<<8)&0xff00ff00);
	val=((val>>16)&0x0000ffff)|((val<<16)&0xffff0000);
	return val;
}

static inline uint16_t ReverseBits16(uint16_t val)
{
	val=((val>>1)&0x5555)|((val<<1)&0xaaaa);
	val=((val>>2)&0x3333)|((val<<2)&0xcccc);
	val=((val>>4)&0x0f0f)|((val<<4)&0xf0f0);
	val=((val>>8)&0x00ff)|((val<<8)&0xff00);
	return val;
}

static inline uint8_t ReverseBits8(uint8_t val)
{
	val=((val>>1)&0x55)|((val<<1)&0xaa);
	val=((val>>2)&0x33)|((val<<2)&0xcc);
	val=((val>>4)&0x0f)|((val<<4)&0xf0);
	return val;
}

static inline int LowestBitSet32(uint32_t val)
{
	if(val==0) return -1;
	return CountBits32(val^(val-1))-1;
}

static inline int LowestBitSet16(uint16_t val)
{
	if(val==0) return -1;
	return CountBits16(val^(val-1))-1;
}

static inline int LowestBitSet8(uint8_t val)
{
	if(val==0) return -1;
	return CountBits8(val^(val-1))-1;
}

#endif
