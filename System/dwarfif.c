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

#define SYSTEM_MODULE_NAME "DWARFIF"

#include <System/types.h>
#include <System/dwarfif.h>
#include <System/task_manager.h>
#include <System/system.h>

int dwarfif_get_section_info(void *obj, Dwarf_Half section_index,
			     Dwarf_Obj_Access_Section *return_section, int *error)
{
	DEBUG_MSG("get section info called (index = %i).\n", section_index);
	task_register_cons *trc = obj;
	
	u_int32_t addr = (u_int32_t)task_get_section_address(trc, section_index);
	Elf32_Shdr *section = (Elf32_Shdr *)((u_int32_t)trc->elfh + trc->elfh->e_shoff) + section_index;
	Elf32_Shdr *strtab  = (Elf32_Shdr *)((u_int32_t)trc->elfh + trc->elfh->e_shoff) + trc->elfh->e_shstrndx;

	char *strtab_addr = (char *)((u_int32_t)trc->elfh + strtab->sh_offset);

	if (addr == 0) {
		return DW_DLV_NO_ENTRY;
	}

	return_section->addr	  = addr;
	return_section->size	  = section->sh_size;
	return_section->name	  = strtab_addr + section->sh_name;
	return_section->link	  = section->sh_link;
	return_section->entrysize = section->sh_entsize;
	
	return DW_DLV_OK;
}

Dwarf_Endianness dwarfif_get_byte_order(void* obj)
{
	DEBUG_MSG("get byte order called.\n");
	return DW_OBJECT_LSB;
}

Dwarf_Small dwarfif_get_length_size(void* obj)
{
	DEBUG_MSG("get length size called.\n");
	return sizeof(u_int32_t);
}

Dwarf_Small dwarfif_get_pointer_size(void* obj)
{
	DEBUG_MSG("get pointer size called.\n");
	return sizeof(void *);
}

Dwarf_Unsigned dwarfif_get_section_count(void* obj)
{
	DEBUG_MSG("get section count called.\n");
	task_register_cons *trc = obj;
	return trc->elfh->e_shnum;
}

int dwarfif_load_section(void* obj, Dwarf_Half section_index, 
			 Dwarf_Small** return_data, int* error)
{
	DEBUG_MSG("load section called.\n");
	task_register_cons *trc = obj;
	void *ret = task_get_section_address(trc, section_index);
	*return_data = ret;

	if (ret == NULL) {
		return DW_DLV_NO_ENTRY;
	}
	return DW_DLV_OK;
}

static const struct Dwarf_Obj_Access_Methods_s dwarfif_access_methods =	
{ dwarfif_get_section_info, 
  dwarfif_get_byte_order,
  dwarfif_get_length_size, 
  dwarfif_get_pointer_size,
  dwarfif_get_section_count, 
  dwarfif_load_section,
  NULL 
};

int dwarfif_get_access_if(task_register_cons *trc, Dwarf_Obj_Access_Interface *aif)
{
	aif->object = trc;
	aif->methods = &dwarfif_access_methods;
	return 1;
}
