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

#ifndef SYSTEM_H
#define SYSTEM_H

#include <System/system_config.h>
#include <umm_malloc.h>

#include <stdio.h>

#ifdef DEBUG
#ifdef SYSTEM_MODULE_NAME
#define DEBUG_MSG(...)	printf(SYSTEM_MODULE_NAME ": DEBUG>>> "  __VA_ARGS__)
#else				/* SYSTEM_MODULE_NAME */
#define DEBUG_MSG(...)	printf("DEBUG>>> "  __VA_ARGS__)
#endif /* SYSTEM_MODULE_NAME */
#else
#define DEBUG_MSG(...)
#endif

#ifdef INFO
#ifdef SYSTEM_MODULE_NAME
#define INFO_MSG(...)	printf(SYSTEM_MODULE_NAME ": INFO >>> " __VA_ARGS__)
#else				/* SYSTEM_MODULE_NAME */
#define INFO_MSG(...)	printf("INFO >>> " __VA_ARGS__)
#endif /* SYSTEM_MODULE_NAME */
#else
#define INFO_MSG(...)
#endif

#ifdef SYSTEM_MODULE_NAME
#define ERROR_MSG(...)	printf(SYSTEM_MODULE_NAME ": ERROR>>> " __VA_ARGS__)
#else				/* SYSTEM_MODULE_NAME */
#define ERROR_MSG(...)	printf("ERROR>>> " __VA_ARGS__)
#endif /* SYSTEM_MODULE_NAME */

#define SYSTEM_MALLOC_CALL(s) pvPortMalloc(s)
#define SYSTEM_FREE_CALL(p) vPortFree(p)

#define TASKSECTION_MALLOC_CALL(s) umm_malloc(s)
#define TASKSECTION_FREE_CALL(p) umm_free(p)

#endif /* SYSTEM_H */
