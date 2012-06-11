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

#define SYSTEM_MODULE_NAME "VX_LOADER"

#include <stdio.h>
#include <string.h>

#include <System/elf.h>
#include <System/types.h>
#include <System/system.h>

#define NUMBER_OF_CORES		4
#define KERNEL_ALLOC_SIZE	0x01000000

typedef void (*entry_point_fun_t)();

static entry_point_fun_t jump_table[NUMBER_OF_CORES];

static void *kernel_locations[NUMBER_OF_CORES];

extern ma_t Vector_Init;
extern ma_t _kernel_elf_start;
extern ma_t _kernel_elf_end;

#define KERNEL_ELFH	((Elf32_Ehdr *)&_kernel_elf_start)
#define KERNEL_ELF_SIZE ((ms_t)((npi_t)&_kernel_elf_end - (npi_t)&_kernel_elf_start))

static int check_elf_magic(Elf32_Ehdr *hdr)
{
	if (hdr->e_ident[0] != ELFMAG0)
		return 0;

	if (hdr->e_ident[1] != ELFMAG1)
		return 0;

	if (hdr->e_ident[2] != ELFMAG2)
		return 0;

	if (hdr->e_ident[3] != ELFMAG3)
		return 0;

	return 1;
}

void allocate_elf_at_offset(Elf32_Ehdr *elfh, void *start_address)
{
	int i;
	Elf32_Shdr *s = (Elf32_Shdr *)((npi_t)elfh + elfh->e_shoff);

	for (i = 0; i < KERNEL_ELFH->e_shnum; i++) {
		if (!(s[i].sh_flags & SHF_ALLOC)) 
			continue;
		
		void *section_addr = (void *)((npi_t)start_address + (npi_t)s[i].sh_addr);
		DEBUG_MSG("Copying section #%i to 0x%x\n", i, (npi_t)section_addr);

		if (s[i].sh_type != SHT_NOBITS) {
			/*
			 * Copy the section if it contains data.
			 */
			memcpy(section_addr,
			       (void *)((npi_t)elfh + (npi_t)s[i].sh_offset),
			       s[i].sh_size);
		} else {
			/*
			 * Set the entire section to zero, if it
			 * doesn't have any data supplied.
			 */
			bzero(section_addr, s[i].sh_size);
		}
	}
}

void load_and_jump()
{
	int i;
	/*
	 * Make software interrupt and jump to the right address.
	 */

	void *image_entry = (void *)&Vector_Init;

	for (i = 0; i < NUMBER_OF_CORES; i++) {
		printf("Copying kernel image %i...\n", i);
		memcpy(kernel_locations[i], image_entry, KERNEL_ALLOC_SIZE);
	}

	asm( "mrc		p15,0,r1,c1,c0,0\n\t"	// Read control register configuration data.
	     "bic		r1,r1,#(1<<0)\n\t"	// Disable MMU.
	     "bic		r1,r1,#(1<<12)\n\t"	// Disable I Cache.
	     "bic		r1,r1,#(1<<2)\n\t"	// Disable D Cache.
	     "mcr		p15,0,r1,c1,c0,0\n\t"	// Write control register configuration data.

	     "ldr		r1,=0x1e001000\n\t"	// ICDDCR
	     "mov		r2,#0x01\n\t"
	     "str		r2,[r1]	\n\t"	// Enable GIC.

	     "ldr		r1,=0x10000000\n\t"
	     "mvn		r2,#0\n\t"
	     "str		r2,[r1,#0x34]\n\t"	// SYS_FLAGSCLR
	     "adr		r2,Launcher\n\t"
	     "str		r2,[r1,#0x30]\n\t"	// SYS_FLAGSSET

	     "ldr		r1,=0x1e001f00\n\t"	// ICDSGIR
	     "ldr		r2,=0x000e0001\n\t"	// Using target list, target cores 1, 2 and 3, interrupt #1.
	     "str		r2,[r1]\n\t"		// Send software interrupt.
	     "Launcher:\n\t"
	     "mrc		p15,0,r2,c0,c0,5\n\t"
	     "and		r2,r2,#3\n\t"			// Find Core ID
	     //"adr		r1,%0\n\t"
	     "ldr		pc,[%0,r2,lsl #2]\n\t"
	     : /* no output operands */
	     : "r" (jump_table)
	     : "r1", "r2");
}

int _init()
{
	if (!check_elf_magic(KERNEL_ELFH)) {
		ERROR_MSG("The kernel elf magic does not check out.\n");
		goto error;
	}
	
	int i;
	npi_t entry_point_address_offset = KERNEL_ELFH->e_entry;

	for (i = 0; i < NUMBER_OF_CORES; i++) {
		INFO_MSG("Copying kernel for core #%i\n", i);
		npi_t start_address = 0x00100000 + KERNEL_ALLOC_SIZE*i;
		allocate_elf_at_offset(KERNEL_ELFH, (void *)start_address);
		jump_table[i]	    = 
			(entry_point_fun_t)(start_address + 
					    entry_point_address_offset);
	}

	jump_table[0]();

	printf("Jaha.\n");
error:
	while (1) 
		;
	return 0;
}


void vApplicationMallocFailedHook( void )
{
	__asm volatile (" smc #0 ");
}

void vApplicationIdleHook( void )
{

}
