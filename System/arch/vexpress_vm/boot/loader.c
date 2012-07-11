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

#define VX_LOADER VX_LOADER
#define SYSTEM_MODULE VX_LOADER

#include <stdio.h>
#include <string.h>

#include <System/applications.h>
#include <System/elf.h>
#include <System/types.h>
#include <System/system.h>
#include <System/pl011.h>

#include <System/arch/vexpress_vm/memory_layout.h>

extern ma_t _kernel_elf_start;
extern ma_t _kernel_elf_end;

extern ma_t _simple_elf_start;
extern ma_t _writer_elf_start;
extern ma_t _reader_elf_start;
extern ma_t _rtuappv1_elf_start;
extern ma_t _rtuappv2_elf_start;
extern ma_t _tunnel_elf_start;
extern ma_t _field_elf_start;

binary_register_entry binary_register[] = {
  { "kernel", APPLICATION_ELF(kernel) },
  { "simple", APPLICATION_ELF(simple) },
  { "tunnel", APPLICATION_ELF(tunnel) },
  { "field", APPLICATION_ELF(field) },
  { NULL, NULL }
};

#define KERNEL_ELFH	((Elf32_Ehdr *)&_kernel_elf_start)
#define KERNEL_ELF_SIZE ((ms_t)((npi_t)&_kernel_elf_end - (npi_t)&_kernel_elf_start))

xMemoryInformationType *mit = MIS_START_ADDRESS;

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

void allocate_elf_at_offset(Elf32_Ehdr *elfh, void *start_address, xMemoryInformationType *mit)
{
	int i;
	Elf32_Shdr *s = (Elf32_Shdr *)((npi_t)elfh + elfh->e_shoff);

	for (i = 0; i < elfh->e_shnum; i++) {
		if (!(s[i].sh_flags & SHF_ALLOC))
			continue;

		void *section_addr = (void *)((npi_t)start_address + (npi_t)s[i].sh_addr);
		DEBUG_MSG("Copying section #%i to 0x%x\n", i, (npi_t)section_addr);

		mit->phys_data_size =
			mit->phys_data_size < s[i].sh_addr + s[i].sh_size
			? s[i].sh_addr + s[i].sh_size
			: mit->phys_data_size;

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
	DEBUG_MSG("Kernel occupying %u bytes from 0x%x to 0x%x\n",
		  (unsigned int)mit->phys_data_size, (npi_t)start_address,
		  (npi_t)(start_address + mit->phys_data_size));
}

void *stack_ps[NUMBER_OF_CORES];
void *entry_ps[NUMBER_OF_CORES];

void launch_kernels()
{
	int i;

	for (i = 0; i < NUMBER_OF_CORES; i++) {
		stack_ps[i] = mit[i].phys_end;
		entry_ps[i] = mit[i].phys_entry_point;
		DEBUG_MSG("Core #%u entry point @ 0x%x\n", (unsigned int)i, (npi_t)mit[i].phys_entry_point);
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

	     "ldr		r1, stack_ps_addr\n\t"
	     "ldr		sp, [r1, r2, lsl #2]\n\t"

	     "ldr		r1, entry_ps_addr\n\t"
	     "ldr		pc, [r1, r2, lsl #2]\n\t"
	     "stack_ps_addr:    .word stack_ps\n\t"
	     "entry_ps_addr:    .word entry_ps\n\t"
	     :
	     :
	     : "r1", "r2" );
}

static void init_uart(void)
{
	int i;

	for (i = 0; i < NUMBER_OF_CORES; i++) {
		vUARTInitialise(i, 38400, 64);
	}
}

int _init()
{
	int i;

	init_uart();

	if (!check_elf_magic(KERNEL_ELFH)) {
		ERROR_MSG("The kernel elf magic does not check out.\n");
		goto error;
	}

	Elf32_Ehdr *kernel_elfh = KERNEL_ELFH;
	npi_t entry_point_address_offset =kernel_elfh->e_entry;

	for (i = 0; i < NUMBER_OF_CORES; i++) {
		INFO_MSG("Copying kernel for core #%i\n", i);
		mit[i].phys_start		  = KSN_START_ADDRESS(i);
		mit[i].phys_end			  = KSN_START_ADDRESS(i+1);
		mit[i].phys_entry_point		  = mit[i].phys_start + entry_point_address_offset;
		mit[i].phys_heap_begin		  = GVMSN_START_ADDRESS(i);
		mit[i].phys_heap_size		  = GVMS_SIZE;
		mit[i].phys_iccs_begin		  = ICCS_START_ADDRESS;
		mit[i].phys_iccs_size		  = ICCS_SIZE;
		mit[i].phys_binary_register_begin = binary_register;
		mit[i].phys_binary_register_size  = sizeof(binary_register);
		allocate_elf_at_offset(KERNEL_ELFH, mit[i].phys_start, &mit[i]);
	}

#ifndef MULTICORE_DISABLED
	launch_kernels();
#else
	asm( "ldr               sp, [%0]\n\t"
	     "ldr               pc, [%1]\n\t"
	     :
	     : "r" (&mit[0].phys_end), "r" (&mit[0].phys_entry_point)
	     : );
#endif


	/*
	 * Not reached.
	 */

error:
	while (1)
		;
	return 0;
}
