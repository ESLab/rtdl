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

ifdef(`simple_app',
_simple_elf_start = ALIGN(0x4);
. = _simple_elf_start;
INCLUDE "build/simple-CONFIG.ld";
_simple_elf_end = .;)

ifdef(`writer_app',
_writer_elf_start = ALIGN(0x4);
. = _writer_elf_start;
INCLUDE "build/writer-CONFIG.ld";
_writer_elf_end = .;)

ifdef(`reader_app',
_reader_elf_start = ALIGN(0x4);
. = _reader_elf_start;
INCLUDE "build/reader-CONFIG.ld";
_reader_elf_end = .;)

ifdef(`rtuappv1_app',
_rtuappv1_elf_start = ALIGN(0x4);
. = _rtuappv1_elf_start;
INCLUDE "build/rtuappv1-CONFIG.ld";
_rtuappv1_elf_end = .;)

ifdef(`rtuappv2_app',
_rtuappv2_elf_start = ALIGN(0x4);
. = _rtuappv2_elf_start;
INCLUDE "build/rtuappv2-CONFIG.ld";
_rtuappv2_elf_end = .;)

ifdef(`tunnel_app',
_tunnel_elf_start = ALIGN(0x4);
. = _tunnel_elf_start;
INCLUDE "build/tunnel-CONFIG.ld";
_tunnel_elf_end = .;)

ifdef(`field_app',
_field_elf_start = ALIGN(0x4);
. = _field_elf_start;
INCLUDE "build/field-CONFIG.ld";
_field_elf_end = .;)

ifdef(`rtucontv1_app',
_rtucontv1_elf_start = ALIGN(0x4);
. = _rtucontv1_elf_start;
INCLUDE "build/rtucontv1-CONFIG.ld";
_rtucontv1_elf_end = .;)

ifdef(`rtucontv2_app',
_rtucontv2_elf_start = ALIGN(0x4);
. = _rtucontv2_elf_start;
INCLUDE "build/rtucontv2-CONFIG.ld";
_rtucontv2_elf_end = .;)

ifdef(`rtucontv3_app',
_rtucontv3_elf_start = ALIGN(0x4);
. = _rtucontv3_elf_start;
INCLUDE "build/rtucontv3-CONFIG.ld";
_rtucontv3_elf_end = .;)
