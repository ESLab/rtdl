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

/*
 * Essentially layer violation to get access to libdwarf internals.
 */
#include <dwarf_incl.h>

int dwarfif_get_section_info(void *obj, Dwarf_Half section_index,
			     Dwarf_Obj_Access_Section *return_section, int *error)
{
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
	return DW_OBJECT_LSB;
}

Dwarf_Small dwarfif_get_length_size(void* obj)
{
	return sizeof(u_int32_t);
}

Dwarf_Small dwarfif_get_pointer_size(void* obj)
{
	return sizeof(void *);
}

Dwarf_Unsigned dwarfif_get_section_count(void* obj)
{
	task_register_cons *trc = obj;
	return trc->elfh->e_shnum;
}

int dwarfif_load_section(void* obj, Dwarf_Half section_index,
			 Dwarf_Small** return_data, int* error)
{
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

int dwarfif_init(task_register_cons *trc, Dwarf_Debug *dbg)
{
	int ret;
	Dwarf_Error error;
	Dwarf_Handler errhand = 0;
	Dwarf_Ptr errarg = 0;
	Dwarf_Obj_Access_Interface *binary_interface;

	binary_interface = SYSTEM_MALLOC_CALL(sizeof(Dwarf_Obj_Access_Interface));
	if (binary_interface == NULL) {
		ERROR_MSG("Could not allocate memory.\n");
		goto error0;
	}

	if (!dwarfif_get_access_if(trc, binary_interface)) {
		ERROR_MSG("Could not get access interface.\n");
		goto error1;
	}

	if ((ret = dwarf_object_init(binary_interface, errhand,
				     errarg, dbg, &error)) != DW_DLV_OK) {
		ERROR_MSG("Could not initiate dwarf object.\n");
		goto error1;
	}

	return 1;

error1:
	SYSTEM_FREE_CALL(binary_interface);
error0:
	return 0;
}

int dwarfif_free(Dwarf_Debug dbg)
{
	Dwarf_Error err;
	return dwarf_object_finish(dbg, &err) == DW_DLV_OK ? 1 : 0;
}

int dwarfif_finish(Dwarf_Debug *dbg)
{
	Dwarf_Error error = 0;
	SYSTEM_FREE_CALL((*dbg)->de_obj_file);

	if (dwarf_object_finish(*dbg, &error) != DW_DLV_OK) {
		ERROR_MSG("Could not finish dwarf object.\n");
		return 0;
	}

	return 1;
}

int dwarfif_get_type_die(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Die *type_die)
{
	Dwarf_Error err;
	Dwarf_Half tag;
	Dwarf_Attribute attr;
	Dwarf_Off off;
	if (dwarf_tag(die, &tag, &err) != DW_DLV_OK) {
		INFO_MSG("Could not get die tag.\n");
		return 0;
	}
	if (tag != DW_TAG_variable) {
		ERROR_MSG("Die is not a variable.\n");
		return 0;
	}
	if (dwarf_attr(die, DW_AT_type, &attr, &err) != DW_DLV_OK) {
		INFO_MSG("Could not get type attribute from variable.\n");
		return 0;
	}
	if (dwarf_global_formref(attr, &off, &err) != DW_DLV_OK) {
		INFO_MSG("Could not get dwarf offset for type die.\n");
		return 0;
	}
	if (dwarf_offdie(dbg, off, type_die, &err) != DW_DLV_OK) {
		INFO_MSG("Could not get type die from offset.\n");
		return 0;
	}
	if (dwarf_tag(*type_die, &tag, &err) != DW_DLV_OK) {
		INFO_MSG("Could not get tag from type die.\n");
		return 0;
	}

	return 1;
}

Dwarf_Die dwarfif_follow_attr(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half follow_attr)
{
	Dwarf_Error	err;
	int		res;
	Dwarf_Attribute attr;
	Dwarf_Off	off;
	Dwarf_Die	new_die;

	if ((res = dwarf_attr(die, follow_attr, &attr, &err)) != DW_DLV_OK) {
		return NULL;
	}

	if (dwarf_global_formref(attr, &off, &err) != DW_DLV_OK) {
		return NULL;
	}

	if (dwarf_offdie(dbg, off, &new_die, &err) != DW_DLV_OK) {
		return NULL;
	}

	return new_die;
}

Dwarf_Die dwarfif_follow_attr_until(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half follow_attr, Dwarf_Half until_tag)
{
	Dwarf_Error	err;
	Dwarf_Half	tag;

	while (1) {

		if (dwarf_tag(die, &tag, &err) != DW_DLV_OK) {
			return NULL;
		}

		if (tag == until_tag) {
			return die;
		}

		die = dwarfif_follow_attr(dbg, die, follow_attr);

		if (die == NULL)
			return NULL;

	}
}

int dwarfif_die_has_typetag(Dwarf_Die die)
{
	Dwarf_Error err;
	Dwarf_Half tag;

	if (dwarf_tag(die, &tag, &err) != DW_DLV_OK) {
		return 0;
	}

	switch (tag) {
	case DW_TAG_array_type:
	case DW_TAG_class_type:
	case DW_TAG_enumeration_type:
	case DW_TAG_pointer_type:
	case DW_TAG_reference_type:
	case DW_TAG_string_type:
	case DW_TAG_structure_type:
	case DW_TAG_subroutine_type:
	case DW_TAG_typedef:
	case DW_TAG_union_type:
	case DW_TAG_ptr_to_member_type:
	case DW_TAG_set_type:
	case DW_TAG_subrange_type:
	case DW_TAG_base_type:
	case DW_TAG_const_type:
	case DW_TAG_file_type:
	case DW_TAG_packed_type:
	case DW_TAG_thrown_type:
	case DW_TAG_volatile_type:
	case DW_TAG_restrict_type:
	case DW_TAG_interface_type:
	case DW_TAG_unspecified_type:
	case DW_TAG_mutable_type:
	case DW_TAG_shared_type:
	case DW_TAG_rvalue_reference_type:
		return 1;
	default:
		return 0;
	}
}

/*
 * This function tries to find the address of a static variable,
 * e.g. the variable is statically allocated in a data section and the
 * address is given in the debug information.
 */
void *dwarfif_find_static_var_address(task_register_cons *trc, Dwarf_Die var)
{
	Dwarf_Error	  err;
	Dwarf_Attribute	  attr;
	Dwarf_Block	 *location_block;
	Dwarf_Locdesc	**ldl;
	Dwarf_Signed	  noe;

	if (dwarf_attr(var, DW_AT_location, &attr, &err) != DW_DLV_OK) {
		return NULL;
	}

	if (dwarf_formblock(attr, &location_block, &err) != DW_DLV_OK) {
		return NULL;
	}

	if (dwarf_loclist_n(attr, &ldl, &noe, &err) != DW_DLV_OK) {
		return NULL;
	}

	Dwarf_Loc *loc = ldl[0]->ld_s;
	if (loc->lr_atom != DW_OP_addr) {
		/*
		 * Unsupported.
		 */
		return NULL;
	}
	u_int32_t offset = loc->lr_number;

	return (void *)((u_int32_t)trc->cont_mem + offset);
}
