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

#include <string.h>
#include <stdio.h>

#include <types.h>
#include <linker.h>
#include <system.h>

int check_elf_magic(Elf32_Ehdr *hdr)
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

entry_ptr_t get_entry_point(Elf32_Ehdr *elfh, Elf32_Sym *entry_symbol)
{
	return (entry_ptr_t)((u_int32_t)elfh + (u_int32_t)(entry_symbol->st_value));
}

char *get_shstr(Elf32_Ehdr *elf_h, Elf32_Shdr *shstr_hdr, Elf32_Word index)
{
	char *data = (char *)((u_int32_t)elf_h + shstr_hdr->sh_offset);

	return data + index;
}

Elf32_Shdr *find_section(char *name, Elf32_Ehdr *elf_h)
{
	int i;
	Elf32_Shdr *sect_hdr = (Elf32_Shdr *)((u_int32_t)elf_h + elf_h->e_shoff);
	Elf32_Shdr *shstr_hdr = &sect_hdr[elf_h->e_shstrndx];
	for (i = 0; i < elf_h->e_shnum; i++) {
		char *a = get_shstr(elf_h, shstr_hdr, sect_hdr[i].sh_name);
		if (strcmp(a, name) == 0)
			return &sect_hdr[i];
	}
	return NULL;
}

Elf32_Sym *get_symtab(Elf32_Ehdr *elf_h)
{
	Elf32_Shdr *sect_hdr = (Elf32_Shdr *)((u_int32_t)elf_h + elf_h->e_shoff);
	sect_hdr = find_section(".symtab", elf_h);

	if (sect_hdr != NULL)
		return (Elf32_Sym *)((u_int32_t)elf_h + (u_int32_t)sect_hdr->sh_offset);
	else
		return NULL;
}

Elf32_Sym *find_symbol(char *name, Elf32_Ehdr *elf_h)
{
	int i;
	char *tname;

	Elf32_Shdr *symtab_sect = find_section(".symtab", elf_h);
	Elf32_Shdr *strtab_sect = find_section(".strtab", elf_h);

	if (symtab_sect == NULL) {
		printf("ERROR Found no .symtab section\n");
		return NULL;
	}

	if (strtab_sect == NULL) {
		printf("ERROR Found no .strtab section\n");
		return NULL;
	}

	if (symtab_sect->sh_entsize != sizeof(Elf32_Sym)) {
		printf("ERROR: Wrong .symtab entry size\n");
		return NULL;
	}

	Elf32_Sym *syms = (Elf32_Sym *)((u_int32_t)elf_h + symtab_sect->sh_offset);

	u_int32_t n = symtab_sect->sh_size / symtab_sect->sh_entsize;
	printf("Found %i entries in .symtab (sect at address 0x%x)\n", n, (unsigned int)syms);
	
	for (i = 0; i < n; i++) {
		tname = get_shstr(elf_h, strtab_sect, syms[i].st_name);
		if (strcmp(tname, name) == 0)
			return &syms[i];
	}
	return NULL;
}

int set_and_allocate_bits_sections(Elf32_Ehdr *elfh)
{
	int i;
	Elf32_Shdr *s = (Elf32_Shdr *)((u_int32_t)elfh + elfh->e_shoff);
	for (i = 0; i < elfh->e_shnum; i++) {
		/*
		 * If the section is a NOBITS section, a.k.a. .bss,
		 * allocate it. Setting the elf sh_addr, usually used
		 * for virtual addresses to the allocated memory
		 * region.
		 *
		 * WARNING: Ignoring section alignment here, and
		 * possibly other attributes.
		 */
		
		if (s[i].sh_type == SHT_NOBITS) {
			u_int32_t allocated_section = (u_int32_t)pvPortMalloc(s[i].sh_size);
			if (allocated_section == 0) {
				printf("ERROR: could not allocate NOBITS section\n");
				return 0;
			}
			s[i].sh_addr = allocated_section;
		}
		
		/*
		 * For PROGBITS segements, set the sh_addr field to
		 * the correct (non-virtual) memory address.
		 */

		if (s[i].sh_type == SHT_PROGBITS) {
			s[i].sh_addr = (u_int32_t)elfh + s[i].sh_offset;
		}
	}
	return 1;
}

void free_bits_sections(Elf32_Ehdr *elfh)
{
	int i;
	Elf32_Shdr *s = (Elf32_Shdr *)((u_int32_t)elfh + elfh->e_shoff);
	for (i = 0; i < elfh->e_shnum; i++) {
		if (s[i].sh_type == SHT_NOBITS) {
			vPortFree((void *)s[i].sh_addr);
		}
	}
}

int find_symbol_in_elfhs(Elf32_Sym *in_symbol, Elf32_Sym **out_symbol, Elf32_Ehdr **out_symbol_elfh,
			 Elf32_Ehdr *app_elfh, Elf32_Ehdr *sys_elfh, Elf32_Ehdr **other_elfhs)
{
	Elf32_Sym *final_symbol = NULL;
	Elf32_Ehdr *final_symbol_elfh;
	Elf32_Shdr *strtab_sect = find_section(".dynstr", app_elfh);
	char *symbol_name = get_shstr(app_elfh, strtab_sect, in_symbol->st_name);
	/* printf("Relocating symbol %s, with symbol index %i\n", symbol_name, ELF32_R_SYM(r[j].r_info)); */
	printf("Relocating symbol %s\n", symbol_name);

	if (in_symbol->st_shndx == SHN_UNDEF) {
		/*
		 * Find the symbol elsewhere.
		 */
		
		Elf32_Ehdr **elfp = other_elfhs;

		if (elfp) {
			while(*elfp) {
				if (*elfp == app_elfh) {
					elfp++;
					continue;
				}
				final_symbol = find_symbol(symbol_name, *elfp);

				if (final_symbol->st_shndx == SHN_UNDEF)
					final_symbol = NULL;

				if (final_symbol != NULL) {
					final_symbol_elfh = *elfp;
					break;
				}
				elfp++;
			}
		}

		if (final_symbol == NULL) {
			final_symbol = find_symbol(symbol_name, sys_elfh);
			final_symbol_elfh = sys_elfh;
		}

	} else {
		final_symbol_elfh = app_elfh;
		final_symbol = find_symbol(symbol_name, final_symbol_elfh);
	}
			
	if (final_symbol == NULL) {
		printf("ERROR: could not link symbol \"%s\"\n", symbol_name);
		return 0;
	}
	
	*out_symbol = final_symbol;
	*out_symbol_elfh = final_symbol_elfh;
	return 1;
}

int link_relocations(Elf32_Ehdr *app_elfh, Elf32_Ehdr *sys_elfh, Elf32_Ehdr **other_elfhs)
{
	int i,j;
	Elf32_Shdr *s = (Elf32_Shdr *)((u_int32_t)app_elfh + app_elfh->e_shoff);
	Elf32_Shdr *app_symsect = find_section(".dynsym", app_elfh);
	Elf32_Shdr *strtab_sect = find_section(".dynstr", app_elfh);

	if (app_symsect == NULL) {
		printf("ERROR: could not find the applications symtab.\n");
		return 0;
	}

	if (strtab_sect == NULL) {
		printf("ERROR: could not find .strtab section in application elf.\n");
		return 0;
	}
	
	Elf32_Sym *app_symtab = (Elf32_Sym *)((u_int32_t)app_elfh + app_symsect->sh_offset);

	/*
	 * First set the correct addresses for the NOBITS and PROGBITS
	 * sections.
	 */

	if (!set_and_allocate_bits_sections(app_elfh)) {
		printf("ERROR: could not set and allocate bits sections.\n");
		return 0;
	}

	/*
	 * For all relocation segments in the application.
	 */
	for (i = 0; i < app_elfh->e_shnum; i++) {
		if (s[i].sh_type != SHT_REL && s[i].sh_type != SHT_RELA)
			continue;
		if (s[i].sh_type == SHT_RELA) {
			printf("ERROR: RELA type segments not supported\n");
			return 0;
		}
		Elf32_Rel *r = (Elf32_Rel *)((u_int32_t)app_elfh + s[i].sh_offset);
		/*
		 * For all relocations in the relocation segment.
		 */
		for (j = 0; j < s[i].sh_size / s[i].sh_entsize; j++) {
			switch (ELF32_R_TYPE(r[j].r_info)) {
			case R_ARM_JUMP_SLOT:
			case R_ARM_GLOB_DAT:
				break;
			default:
				printf("ERROR: found non supported relocation type (%i)\n", ELF32_R_TYPE(r[j].r_info));
				return 0;

			}

			/*
			 * 1. Find the symbol in the given elf binaries.
			 */
			
			Elf32_Sym *app_symbol = &app_symtab[ELF32_R_SYM(r[j].r_info)];
			Elf32_Ehdr *symbol_elf;
			Elf32_Sym *final_symbol;

			if (!find_symbol_in_elfhs(app_symbol, &final_symbol, &symbol_elf,
						  app_elfh, sys_elfh, other_elfhs)) {
				printf("ERROR: could not locate symbol\n");
				return 0;
			}

			/*
			 * 2. Find the absolute address of the symbol.
			 */

			u_int32_t address;

			if (final_symbol->st_shndx == SHN_ABS) {
				address = final_symbol->st_value;
			} else if (symbol_elf == app_elfh) {
				Elf32_Shdr *symbol_section = &s[final_symbol->st_shndx];
				address = final_symbol->st_value - symbol_section->sh_offset 
					+ symbol_section->sh_addr;
			} else if (symbol_elf == sys_elfh) {
				/* extern u_int8_t _text; */
				/* address = (u_int32_t)&_text + final_symbol->st_value; */
				address = final_symbol->st_value;
			} else {
				address = (u_int32_t)symbol_elf + (u_int32_t)final_symbol->st_value;
			}
			
			/*
			 * 3. Set the relocation address.
			 */

			switch (ELF32_R_TYPE(r[j].r_info)) {
			case R_ARM_JUMP_SLOT:
			{
				u_int32_t *rel_address = (u_int32_t *)((u_int32_t)app_elfh + r[j].r_offset);
				*rel_address = (u_int32_t)address;
				
				printf("R_ARM_JUMP_SLOT: The relocation address is set to 0x%x\n", address);
			}
			break;
			case R_ARM_GLOB_DAT:
			{
				u_int32_t *rel_address = (u_int32_t *)((u_int32_t)app_elfh + r[j].r_offset);
				printf("R_ARM_GLOB_DAT: rel_address = 0x%x, address = 0x%x\n", 
				       (unsigned int)rel_address, (unsigned int)address);
				*rel_address = (u_int32_t)address;
				//*(u_int32_t *)address = (u_int32_t)rel_address;
				
				printf("R_ARM_GLOB_DAT: The relocation address is set to 0x%x\n", address);
			}
			break;	
			default:
				return 0; /* not reached */
			}

		}
	}
	

	return 1;
}

int free_relocations(Elf32_Ehdr *elfh)
{
	free_bits_sections(elfh);
	return 1;
}
