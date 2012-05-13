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

#define SYSTEM_MODULE_NAME "LINKER"

#include <string.h>
#include <stdio.h>

#include <System/types.h>
#include <System/linker.h>
#include <System/system.h>

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
		ERROR_MSG("Found no .symtab section\n");
		return NULL;
	}

	if (strtab_sect == NULL) {
		ERROR_MSG("Found no .strtab section\n");
		return NULL;
	}

	if (symtab_sect->sh_entsize != sizeof(Elf32_Sym)) {
		ERROR_MSG("Wrong .symtab entry size\n");
		return NULL;
	}

	Elf32_Sym *syms = (Elf32_Sym *)((u_int32_t)elf_h + symtab_sect->sh_offset);

	u_int32_t n = symtab_sect->sh_size / symtab_sect->sh_entsize;
	INFO_MSG("Found %i entries in .symtab (sect at address 0x%x)\n", n, (unsigned int)syms);

	for (i = 0; i < n; i++) {
		tname = get_shstr(elf_h, strtab_sect, syms[i].st_name);
		if (strcmp(tname, name) == 0)
			return &syms[i];
	}
	return NULL;
}

int find_symbol_in_elfhs(Elf32_Sym *in_symbol, Elf32_Sym **out_symbol, task_register_cons **out_symbol_trc,
			 task_register_cons *app_trc, Elf32_Ehdr *sys_elfh, task_register_cons *other_trcs)
{
	Elf32_Sym *final_symbol = NULL;
	task_register_cons *final_symbol_trc;
	Elf32_Shdr *strtab_sect = find_section(".dynstr", app_trc->elfh);
	char *symbol_name = get_shstr(app_trc->elfh, strtab_sect, in_symbol->st_name);
	INFO_MSG("Relocating symbol %s\n", symbol_name);

	if (in_symbol->st_shndx == SHN_UNDEF) {
		/*
		 * Find the symbol elsewhere.
		 */

		if (other_trcs) {

			struct task_register_cons_t *trcp;

			LIST_HEAD(task_register_list_t, task_register_cons_t)
				trc_list = { other_trcs };

			LIST_FOREACH(trcp, &trc_list, tasks) {
				if (trcp == app_trc)
					continue;
				find_symbol(symbol_name, trcp->elfh);
				if (final_symbol->st_shndx == SHN_UNDEF)
					final_symbol = NULL;
				if (final_symbol != NULL) {
					final_symbol_trc = trcp;
					break;
				}
			}
		}

		if (final_symbol == NULL) {
			final_symbol = find_symbol(symbol_name, sys_elfh);
			/*
			 * If the symbol is found in the system elfh,
			 * let the symbol trc be NULL.
			 */
			final_symbol_trc = NULL;
		}

	} else {
		final_symbol_trc = app_trc;
		final_symbol = find_symbol(symbol_name, final_symbol_trc->elfh);
	}

	if (final_symbol == NULL) {
		ERROR_MSG("Could not link symbol \"%s\"\n", symbol_name);
		return 0;
	}

	*out_symbol = final_symbol;
	*out_symbol_trc = final_symbol_trc;
	return 1;
}

int link_relocations(task_register_cons *app_trc, Elf32_Ehdr *sys_elfh, task_register_cons *other_trcs)
{
	int i,j;
	Elf32_Shdr *s = (Elf32_Shdr *)((u_int32_t)app_trc->elfh + app_trc->elfh);
	Elf32_Shdr *app_symsect = find_section(".dynsym", app_trc->elfh);
	Elf32_Shdr *strtab_sect = find_section(".dynstr", app_trc->elfh);
	Elf32_Shdr *section_sect = (Elf32_Shdr *)((u_int32_t)app_trc->elfh + app_trc->elfh->e_shoff);


	if (app_symsect == NULL) {
		ERROR_MSG("Could not find the applications symtab.\n");
		return 0;
	}

	if (strtab_sect == NULL) {
		ERROR_MSG("Could not find .strtab section in application elf.\n");
		return 0;
	}

	Elf32_Sym *app_symtab = (Elf32_Sym *)((u_int32_t)app_trc->elfh + app_symsect->sh_offset);

	/*
	 * For all relocation segments in the application.
	 */
	for (i = 0; i < app_trc->elfh->e_shnum; i++) {
		if (s[i].sh_type != SHT_REL && s[i].sh_type != SHT_RELA)
			continue;
		if (s[i].sh_type == SHT_RELA) {
			ERROR_MSG("RELA type segments not supported\n");
			return 0;
		}
		Elf32_Rel *r = (Elf32_Rel *)((u_int32_t)app_trc->elfh + s[i].sh_offset);
		/*
		 * For all relocations in the relocation segment.
		 */
		for (j = 0; j < s[i].sh_size / s[i].sh_entsize; j++) {
			switch (ELF32_R_TYPE(r[j].r_info)) {
			case R_ARM_JUMP_SLOT:
			case R_ARM_GLOB_DAT:
				break;
			default:
				ERROR_MSG("Found non supported relocation type (%i)\n", ELF32_R_TYPE(r[j].r_info));
				return 0;

			}

			/*
			 * 1. Find the symbol in the given elf binaries.
			 */

			Elf32_Sym *app_symbol = &app_symtab[ELF32_R_SYM(r[j].r_info)];
			task_register_cons *symbol_trc;
			Elf32_Sym *final_symbol;

			if (!find_symbol_in_elfhs(app_symbol, &final_symbol, &symbol_trc,
						  app_trc, sys_elfh, other_trcs)) {
				ERROR_MSG("Could not locate symbol\n");
				return 0;
			}

			/*
			 * 2. Find the absolute address of the symbol.
			 */

			u_int32_t address;

			if (final_symbol->st_shndx == SHN_ABS) {
				address = final_symbol->st_value;
			} else if (symbol_trc == NULL) {
				/*
				 * If the symbol_trc is NULL the
				 * symbol resides in the system
				 * binary.
				 */
				address = final_symbol->st_value;
			} else if (section_sect[final_symbol->st_shndx].sh_flags & SHF_ALLOC) {
				/*
				 * If the SHF_ALLOC flag is set, the
				 * section should already be allocated
				 * somewhere.
				 */
				address = (u_int32_t)symbol_trc->cont_mem + final_symbol->st_value;
			} else {
				/*
				 * Else we fall back on addressing the symbol in the elf.
				 */
				address = (u_int32_t)symbol_trc->elfh +
					(u_int32_t)final_symbol->st_value;
			}

			/*
			 * 3. Set the relocation address.
			 */

			switch (ELF32_R_TYPE(r[j].r_info)) {
			case R_ARM_JUMP_SLOT:
			{
				u_int32_t *rel_address = (u_int32_t *)((u_int32_t)app_trc->elfh + r[j].r_offset);
				*rel_address = (u_int32_t)address;

				INFO_MSG("R_ARM_JUMP_SLOT: The relocation address is set to 0x%x\n", address);
			}
			break;
			case R_ARM_GLOB_DAT:
			{
				u_int32_t *rel_address = (u_int32_t *)((u_int32_t)app_trc->elfh + r[j].r_offset);
				DEBUG_MSG("R_ARM_GLOB_DAT: rel_address = 0x%x, address = 0x%x\n",
				       (unsigned int)rel_address, (unsigned int)address);
				*rel_address = (u_int32_t)address;
				//*(u_int32_t *)address = (u_int32_t)rel_address;

				INFO_MSG("R_ARM_GLOB_DAT: The relocation address is set to 0x%x\n", address);
			}
			break;
			default:
				return 0; /* not reached */
			}

		}
	}


	return 1;
}
