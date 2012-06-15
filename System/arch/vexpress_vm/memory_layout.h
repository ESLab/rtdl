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

#ifndef VEXPRESS_MEMORY_LAYOUT_H
#define VEXPRESS_MEMORY_LAYOUT_H

#include <FreeRTOS.h>

#include <System/types.h>

typedef struct iccs_layout_t {
	u_int8_t  rcu_section[4][65536];
	u_int32_t notifications[4];
} iccs_layout;

/*
 * Physical memory layout:
 * Page 0: MIS (@ configMIS_SECTION_ADDRESS)
 * Page 1: KS0
 * ...
 * Page *: KS2
 * ...
 * ...
 * Page *: KS3
 * ...
 * Page *: GVMS0
 * ...
 * Page *: GVMS3
 * ...
 * Page *: ICCS
 */

#define NUMBER_OF_CORES	(portMAX_CORE_ID + 1)
#define VM_PAGESIZE		(0x100000)
#define ADDRESS_SPACE_START	configMIS_SECTION_ADDRESS

#define MIS_PAGENO		(0)
#define MIS_PAGES		(1)
#define KS_START_PAGENO	(MIS_PAGENO + MIS_PAGES)
#define KS_PAGES		(10)
#define GVMS_START_PAGENO	(KS_START_PAGENO + KS_PAGES*NUMBER_OF_CORES)
#define GVMS_PAGES		(1)
#define ICCS_START_PAGENO	(GVMS_START_PAGENO + GVMS_PAGES*NUMBER_OF_CORES)
#define ICCS_PAGES		(1)

#define MIS_START_ADDRESS	ADDRESS_SPACE_START
#define MIS_SIZE		(VM_PAGESIZE*MIS_PAGES)
#define MIS_TOTAL_SIZE		MIS_SIZE
#define KS_START_ADDRESS	(ADDRESS_SPACE_START + VM_PAGESIZE*KS_START_PAGENO)
#define KS_SIZE		(VM_PAGESIZE*KS_PAGES)
#define KS_TOTAL_SIZE		(VM_PAGESIZE*KS_PAGES*NUMBER_OF_CORES)
#define GVMS_START_ADDRESS	(ADDRESS_SPACE_START + VM_PAGESIZE*GVMS_START_PAGENO)
#define GVMS_SIZE		(VM_PAGESIZE*GVMS_PAGES)
#define GVMS_TOTAL_SIZE	(VM_PAGESIZE*GVMS_PAGES*NUMBEROF_CORES)
#define ICCS_START_ADDRESS	(ADDRESS_SPACE_START + VM_PAGESIZE*ICCS_START_PAGENO)
#define ICCS_SIZE		(VM_PAGESIZE*ICCS_PAGES)
#define ICCS_TOTAL_SIZE	ICCS_SIZE

#define KSN_START_ADDRESS(n)	(KS_START_ADDRESS + KS_SIZE*(n))
#define GVMSN_START_ADDRESS(n)	(GVMS_START_ADDRESS + GVMS_SIZE*(n))

#endif /* VEXPRESS_MEMORY_LAYOUT_H */
