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

#define A9_LOADER A9_LOADER
#define SYSTEM_MODULE A9_LOADER

#include <stdio.h>
#include <string.h>

#include <System/arch/arm9_novm_bl/memory_layout.h>

#include <System/applications.h>
#include <System/elf.h>
#include <System/types.h>
#include <System/system.h>
#include <System/pl011.h>

extern ma_t _kernel_elf_start;
extern ma_t _kernel_elf_end;

#define KERNEL_ELFH	((Elf32_Ehdr *)&_kernel_elf_start)
#define KERNEL_ELF_SIZE ((ms_t)((npi_t)&_kernel_elf_end - (npi_t)&_kernel_elf_start))

__attribute__ ((section (".binary_register"))) binary_register_entry binary_register[] = {
  { "kernel", APPLICATION_ELF(kernel) },
#ifdef APP_SIMPLE_INCLUDED
  { "simple", APPLICATION_ELF(simple) },
#endif /* APP_SIMPLE_INCLUDED */
#ifdef APP_RTUAPPV1_INCLUDED
  { "rtuappv1", APPLICATION_ELF(rtuappv1) },
#endif /* APP_RTUAPPV1_INCLUDED */
#ifdef APP_RTUAPPV2_INCLUDED
  { "rtuappv2", APPLICATION_ELF(rtuappv2) },
#endif /* APP_RTUAPPV1_INCLUDED */
  { NULL, NULL }
};

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

	for (i = 0; i < elfh->e_shnum; i++) {
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

int main()
{
	if (!check_elf_magic(KERNEL_ELFH)) {
		ERROR_MSG("The kernel elf magic does not check out.\n");
		goto error;
	}

	Elf32_Ehdr		*kernel_elfh	    = KERNEL_ELFH;
	npi_t			 kernel_entry_point = kernel_elfh->e_entry;

	allocate_elf_at_offset(KERNEL_ELFH, 0x0);
	
	/*
	 * Set up the kernels stack somewhere??
	 */

	asm ( "ldr pc, [%0]\n\t"
	      :
	      : "r" (&kernel_entry_point)
	      : );

error:
	while (1)
		;
	return 0;
}
