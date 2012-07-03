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

#define VX_VM_SETUP VX_VM_SETUP
#define SYSTEM_MODULE VX_VM_SETUP

#include <FreeRTOS.h>

#include <System/arch/vexpress_vm/memory_layout.h>

#include <System/types.h>
#include <System/system.h>

static unsigned long page_table[4096] __attribute__ ((aligned (16384)));

/*
 * Descriptors for cached memory pages and uncached memory pages, for
 * more information see:
 * - ARM11 MPCore Processor Technical Manual 5-41
 * - ARM11 MPCore Processor Technical Manual 5-52
 */

#define CACHED_DESCRIPTOR(phys_addr)	(((phys_addr) & 0xfff00000) | 0x05de6)
#define UNCACHED_DESCRIPTOR(phys_addr)	(((phys_addr) & 0xfff00000) | 0x00de2)

#define PAGENO_TO_ADDRESS(pageno)	((pageno) << 20)
#define ADDRESS_TO_PAGENO(address)	(((npi_t)address) >> 20)

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

static void invalidate_instruction_cache()
{
	/* Invalidate instruction cache. */
	__asm volatile ("mcr   p15, 0, %[zero], c7, c5, 0"
			:
			: [zero] "r" (0)
			:
		);
}

static void invalidate_data_cache()
{
	int i,j;
	int cachesets;
	unsigned long CSIR;

	/* Invalidate data cache. */
	__asm volatile("mrc p15, 1, %[csir], c0, c0, 0"
		       :[csir] "=r" (CSIR)
		       :
		       :
		);

	cachesets=((CSIR>>13)&0x1ff)+1;

	for(j = 0; j < 4; j++) {
		for(i = 0; i < cachesets; i++) {
			unsigned long line=(j<<30)|(i<<5);
			__asm volatile ("mcr   p15, 0, %[line], c7, c6, 2"::[line] "r" (line):);
		}
	}
}

#if 0
static void clean_data_cache(void)
{

/************************************************************************/
/* Copied from linux arch/arm/mm/cache-v7.S,			        */
/* this is the text from files header:				        */
/* 								        */
/* linux/arch/arm/mm/cache-v7.S				        */
/* 								        */
/* Copyright (C) 2001 Deep Blue Solutions Ltd.			        */
/* Copyright (C) 2005 ARM Ltd.					        */
/* 								        */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License version 2 as   */
/* published by the Free Software Foundation.			        */
/* 								        */
/* This is the "shell" of the ARMv7 processor support.		        */
/************************************************************************/

	asm("dmb\n\t" /* ensure ordering with previous memory accesses */
	    "mrc	p15, 1, r0, c0, c0, 1\n\t" /* read clidr */
	    "ands	r3, r0, #0x7000000\n\t" /* extract loc from clidr */
	    "mov	r3, r3, lsr #23\n\t" /* left align loc bit field */
	    "beq	finished\n\t" /* if loc is 0, then no need to clean */
	    "mov	r10, #0\n\t" /* start clean at cache level 0 */
	    "loop1:\n\t"
	    "add	r2, r10, r10, lsr #1\n\t" /* work out 3x current cache level */
	    "mov	r1, r0, lsr r2\n\t" /* extract cache type bits from clidr */
	    "and	r1, r1, #7\n\t" /* mask of the bits for current cache only */
	    "cmp	r1, #2\n\t" /* see what cache we have at this level */
	    "blt	skip\n\t" /* skip if no cache, or just i-cache */
	    /* Should be here: Disable interrupts ... */ /* make cssr&csidr read atomic */
	    "mcr	p15, 2, r10, c0, c0, 0\n\t" /* select current cache level in cssr */
	    "isb\n\t" /* isb to sych the new cssr&csidr */
	    "mrc	p15, 1, r1, c0, c0, 0\n\t" /* read the new csidr */
	    /* Should be here: Enable interrupts ... */
	    "and	r2, r1, #7\n\t" /* extract the length of the cache lines */
	    "add	r2, r2, #4\n\t" /* add 4 (line length offset) */
	    "ldr	r4, =0x3ff\n\t"
	    "ands	r4, r4, r1, lsr #3\n\t" /* find maximum number on the way size */
	    "clz	r5, r4\n\t" /* find bit position of way size increment */
	    "ldr	r7, =0x7fff\n\t"
	    "ands	r7, r7, r1, lsr #13\n\t" /* extract max number of the index size */
	    "loop2:\n\t"
	    "mov	r9, r4\n\t" /* create working copy of max way size */
	    "loop3:\n\t"
	    "orr	r6, r10, r9, lsl r5\n\t" /* factor way and cache number into r6 */
	    "orr	r6, r6, r7, lsl r2\n\t" /* factor index number into r6 */
	    "mcr	p15, 0, r6, c7, c14, 2\n\t" /* invalidate by set/way */
	    "subs	r9, r9, #1\n\t" /* decrement the way */
	    "bge	loop3\n\t"
	    "subs	r7, r7, #1\n\t" /* decrement the index */
	    "bge	loop2\n\t"
	    "skip:\n\t"
	    "add	r10, r10, #2\n\t" /* increment cache number */
	    "cmp	r3, r10\n\t"
	    "bgt	loop1\n\t"
	    "finished:\n\t"
	    "mov	r10, #0\n\t" /* switch back to cache level 0 */
	    "mcr	p15, 2, r10, c0, c0, 0\n\t" /* select current cache level in cssr */
	    "dsb\n\t"
	    "isb\n\t"
	    :
	    :
	    : "r0", "r1", "r2",
	      "r3", "r4", "r5",
	      "r6", "r7", "r9",
	      "r10"
		);
}
#endif

void invalidate_TLB()
{
	/* Invalidate TLB. */
	__asm volatile ("mcr   p15, 0, %[zero], c8, c7, 0"
			:
			:[zero] "r" (0)
			:
		);
}

/*
 * Initiation for vm setup, copied from port.c
 */

static void setup_vm_init(void)
{
	/* unsigned long CSIR; */
	/* int cachesets; */
	/* int i,j; */

	WriteSCTLR(ReadSCTLR()&~(1<<0));

	invalidate_instruction_cache();

	invalidate_data_cache();

	invalidate_TLB();

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

	for (; i < KS_PAGES; i++) {
		page_table[i] = CACHED_DESCRIPTOR((npi_t)KSN_START_ADDRESS(portCORE_ID()) + VM_PAGESIZE*i);
	}

	/*
	 * Hardware stuff and reserved regions are uncached. (up to 0x60000000)
	 */

	DEBUG_MSG("Uncached memory starts @ physical 0x%x.\n", (npi_t)i*VM_PAGESIZE);
	for (; i < 1536; i++) {
		page_table[i] = UNCACHED_DESCRIPTOR(i*VM_PAGESIZE);
	}
	DEBUG_MSG("Uncached memory ends @ physical 0x%x.\n", (npi_t)i*VM_PAGESIZE);

	for (; i < (npi_t)ADDRESS_TO_PAGENO(configMIS_SECTION_ADDRESS) + ICCS_START_PAGENO; i++) {
		page_table[i] = CACHED_DESCRIPTOR(i*VM_PAGESIZE);
	}

	DEBUG_MSG("Uncached ICCS starts @ physical 0x%x\n", (npi_t)(i*VM_PAGESIZE));
	for (; i < 4096; i++) {
		page_table[i] = UNCACHED_DESCRIPTOR(i*VM_PAGESIZE);
	}

	setup_vm_finish();
}
