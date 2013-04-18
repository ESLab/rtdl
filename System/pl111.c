/***********************************************************************************/
/* Copyright (c) 2013, Dag Ågren. All rights reserved.				   */
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

#include "pl111.h"

#define CLCDBase (0x10000000+0x1f000)

#define LCDTiming0 (*(volatile uint32_t *)(CLCDBase+0x00))
#define LCDTiming1 (*(volatile uint32_t *)(CLCDBase+0x04))
#define LCDTiming2 (*(volatile uint32_t *)(CLCDBase+0x08))
#define LCDTiming3 (*(volatile uint32_t *)(CLCDBase+0x0c))
#define LCDUPBASE (*(volatile uint32_t *)(CLCDBase+0x10))
#define LCDLPBASE (*(volatile uint32_t *)(CLCDBase+0x14))
#define LCDControl (*(volatile uint32_t *)(CLCDBase+0x18))
#define LCDIMSC (*(volatile uint32_t *)(CLCDBase+0x1c))
#define LCDRIS (*(volatile uint32_t *)(CLCDBase+0x20))
#define LCDMIS (*(volatile uint32_t *)(CLCDBase+0x24))
#define LCDICR (*(volatile uint32_t *)(CLCDBase+0x28))
#define LCDUPCURR (*(volatile uint32_t *)(CLCDBase+0x2c))
#define LCDLPCURR (*(volatile uint32_t *)(CLCDBase+0x30))
#define LCDPaletteBase ((volatile uint32_t *)(CLCDBase+0x200))
#define CursorImageBase ((volatile uint8_t *)(CLCDBase+0x800))
#define ClcdCrsrCtrl (*(volatile uint32_t *)(CLCDBase+0xc00))
#define ClcdCrsrConfig (*(volatile uint32_t *)(CLCDBase+0xc04))
#define ClcdCrsrPalette0 (*(volatile uint32_t *)(CLCDBase+0xc08))
#define ClcdCrsrPalette1 (*(volatile uint32_t *)(CLCDBase+0xc0c))
#define ClcdCrsrXY (*(volatile uint32_t *)(CLCDBase+0xc10))
#define ClcdCrsrClip (*(volatile uint32_t *)(CLCDBase+0xc14))
#define ClcdCrsrIMSC (*(volatile uint32_t *)(CLCDBase+0xc20))
#define ClcdCrsrICR (*(volatile uint32_t *)(CLCDBase+0xc24))
#define ClcdCrsrRIS (*(volatile uint32_t *)(CLCDBase+0xc28))
#define ClcdCrsrMIS (*(volatile uint32_t *)(CLCDBase+0xc2c))
#define CLCDPeriphID0 (*(volatile uint32_t *)(CLCDBase+0xfe0))
#define CLCDPeriphID1 (*(volatile uint32_t *)(CLCDBase+0xfe4))
#define CLCDPeriphID2 (*(volatile uint32_t *)(CLCDBase+0xfe8))
#define CLCDPeriphID3 (*(volatile uint32_t *)(CLCDBase+0xfec))
#define CLCDCellID0 (*(volatile uint32_t *)(CLCDBase+0xff0))
#define CLCDCellID1 (*(volatile uint32_t *)(CLCDBase+0xff4))
#define CLCDCellID2 (*(volatile uint32_t *)(CLCDBase+0xff8))
#define CLCDCellID3 (*(volatile uint32_t *)(CLCDBase+0xffc))

void InitializeScreen640x480(int colourdepth,void *framebuffer)
{
	// Disable display.
	LCDControl=0x00010820;

	// Set up framebuffer.
	LCDUPBASE=(uint32_t)framebuffer&~0x7;

	// Horizontal back porch: 64
	// Horizontal front porch: 16
	// Horizontal sync pulse width: 80
	// Pixels per line: 640 (39)
	LCDTiming0=0x3f0f4f9c;

	// Vertical back porch: 0x0d
	// Vertical front porch: 3
	// Vertical sync pulse width: 4
	// Lines per panel: 480
	LCDTiming1=0x0d030ddf;

	// Panel clock divider: 0
	// Bypass panel block divider: yes
	// Clocks per line: 639
	// Invert output enable: no
	// Invert panel clock: yes
	// Invert horizontal synchronization: yes
	// Invert vertical synchronization: yes
	// AC bias pin frequency: 0
	// CLKSEL: 0
	LCDTiming2=0x067f3800;

	// Line end enable: no
	// Line-end signal delay: 0
	LCDTiming3=0x00000000;

	// DMA FIFO watermark level: 8 (1)
	// Generate interrupt at: start of vertical sync
	// LCD power enable: yes
	// Big-endian pixel ordering within byte: no
	// Big-endian byte ordering: no
	// RGB or BGR: RGB (0)
	// Dual panel: no
	// Monochrome LCD: no
	// TFT: yes
	// Monochrome STN LCD: no
	// Bits per pixel: user provided
	// CLCDC Enable: yes
	LCDControl=0x00010821|(colourdepth<<1);

	// Set up framebuffer.
	LCDUPBASE=(uint32_t)framebuffer;
}

void SetScreenFrameBuffer(void *framebuffer)
{
	LCDUPBASE=(uint32_t)framebuffer&~0x7;
}

void SetScreenPalette(int index,int r,int g,int b)
{
	if(index<0 || index>255) return;
	if(r<0) r=0; if(r>255) r=255;
	if(g<0) g=0; if(g>255) g=255;
	if(b<0) b=0; if(b>255) b=255;

	int intensity=0;
	if(g&4) intensity=1;
	else if((r&4) && (b&4)) intensity=1;

	uint32_t entry=(intensity<<15)|((b>>3)<<10)|((g>>3)<<5)|(r>>3);

	if(index&1)
	{
		LCDPaletteBase[index>>1]=(LCDPaletteBase[index>>1]&0x0000ffff)|(entry<<16);
	}
	else
	{
		LCDPaletteBase[index>>1]=(LCDPaletteBase[index>>1]&0xffff0000)|entry;
	}
}


