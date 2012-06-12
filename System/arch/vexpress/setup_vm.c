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

#include <FreeRTOS.h>

#include <System/arch/vexpress/memory_layout.h>

static unsigned long page_table[4096] __attribute__ ((aligned (16384)));

#define CACHED_DESCRIPTOR(phys_addr)	(((phys_addr) & 0xfff00000) | 0x05de6)
#define UNCACHED_DESCRIPTOR(phys_addr)	(((phys_addr) & 0xfff00000) | 0x00de2)

/*
 * Copied from port.c
 */

static unsigned long ReadSCTLR()
{
	unsigned long SCTLR;
	__asm volatile("mrc p15, 0, %[sctlr], c1, c0, 0"
	:[sctlr] "=r" (SCTLR)::);
	return SCTLR;
}

static void WriteSCTLR(unsigned long SCTLR)
{
	__asm volatile("mcr p15, 0, %[sctlr], c1, c0, 0"
	::[sctlr] "r" (SCTLR):);
}

static unsigned long ReadACTLR()
{
	unsigned long ACTLR;
	__asm volatile("mrc p15, 0, %[actlr], c1, c0, 1"
	:[actlr] "=r" (ACTLR)::);
	return ACTLR;
}

static void WriteACTLR(unsigned long ACTLR)
{
	__asm volatile("mcr p15, 0, %[actlr], c1, c0, 0"
	::[actlr] "r" (ACTLR):);
}

/*
 * Initiation for vm setup, copied from port.c
 */

static void setup_vm_init(void)
{
	unsigned long CSIR;
	int cachesets;
	int i,j;

	WriteSCTLR(ReadSCTLR()&~(1<<0));

	// Invalidate instruction cache.
	__asm volatile ("mcr   p15, 0, %[zero], c7, c5, 0"::[zero] "r" (0):);

	// Invalidate data cache.
	__asm volatile("mrc p15, 1, %[csir], c0, c0, 0"
	:[csir] "=r" (CSIR)::);
	cachesets=((CSIR>>13)&0x1ff)+1;

	for(j=0;j<4;j++) {
		for(i=0;i<cachesets;i++) {
			unsigned long line=(j<<30)|(i<<5);
			__asm volatile ("mcr   p15, 0, %[line], c7, c6, 2"::[line] "r" (line):);
		}
	}

	// Invalidate TLB.
	__asm volatile ("mcr   p15, 0, %[zero], c8, c7, 0"::[zero] "r" (0):);

	// Enable branch prediction.
	WriteSCTLR(ReadSCTLR()|(1<<11));

	// Enable D-side prefetch.
	WriteACTLR(ReadACTLR()|(1<<2));
}

/*
 * Finalization for vm setup, copied from port.c
 */

static void setup_vm_finish(void)
{
	// Initialize MMU.

	// Enable TTB0 only.
	// Set translation table base address.
	// Set all domains to client.
	__asm volatile (
		" mcr	p15, 0, %[ttbcr], c2, c0, 2		\n" // Write Translation Table Base Control Register  LDR   r0, ttb_address
		" mcr	p15, 0, %[ttbr0], c2, c0, 0	\n" // Write Translation Table Base Register 0
		" mcr	p15, 0, %[dacr], c3, c0, 0 	\n" // Write Domain Access Control Register
		: : [ttbcr] "r" (0),
		[ttbr0] "r" (page_table),
		[dacr] "r" (0x55555555)
		:  );

	// Enable MMU.
	WriteSCTLR(ReadSCTLR()|(1<<0));

	// Enable L1 I & D caches.
	WriteSCTLR(ReadSCTLR()|(1<<2)|(1<<12));
}

/*
 * Setup virtual memory.
 */

void setup_vm(void)
{
	setup_vm_init();

	int i = 0;

	/*
	 * Virtual mapping for the kernel.
	 */

	for (; i < KERNEL_ALLOC_PAGES; i++) {
		page_table[i] = CACHED_DESCRIPTOR((unsigned int)KERNEL_START_ADDRESS(portCORE_ID()) + MIS_PAGESIZE*i);
	}
	/*
	 * The rest is mapped 1:1.
	 */
	for (; i < 64; i++) {
		page_table[i] = CACHED_DESCRIPTOR(i*MIS_PAGESIZE);
	}

	/*
	 * Hardware stuff and reserved regions are uncached. (up to 0x60000000)
	 */

	for (; i < 1536; i++) {
		page_table[i] = UNCACHED_DESCRIPTOR(i*MIS_PAGESIZE);
	}

	for (; i < 4096; i++) {
		page_table[i] = CACHED_DESCRIPTOR(i*MIS_PAGESIZE);
	}

	setup_vm_finish();
}
