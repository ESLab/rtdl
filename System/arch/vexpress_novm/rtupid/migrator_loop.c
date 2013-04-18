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

#define VX_MIGRATOR_LOOP VX_MIGRATOR_LOOP
#define SYSTEM_MODULE VX_MIGRATOR_LOOP

#include <FreeRTOS.h>

#include <System/system.h>
#include <System/task_manager.h>
#include <System/migrator.h>
#include <System/applications.h>

int migrator_loop()
{
	task_register_cons *trc;

	while (1) {
#if 0
		vTaskDelay(20000/portTICK_RATE_MS);

		if ((trc = task_find("rtucont"))) {
			if (!task_wait_for_checkpoint(trc, cp_req_rtu)) {
				ERROR_MSG("%s: Failed to reach rtu checkpoint for task \"%s\"\n",
					  __func__, trc->name);
				return 0;
			}

			Elf32_Ehdr *new_sw = (Elf32_Ehdr *)&_rtucontv2_elf_start;

			INFO_MSG("Starting runtime update.\n");
			if (!runtime_update(trc, new_sw)) {
				ERROR_MSG("Runtime updating failed.\n");
				return 0;
			}
			INFO_MSG("Runtime update complete. (-> v2)\n");
		} else {
			return 0;
		}
#endif
		vTaskDelay(20000/portTICK_RATE_MS);

		if ((trc = task_find("rtucont"))) {
			if (!task_wait_for_checkpoint(trc, cp_req_rtu)) {
				ERROR_MSG("%s: Failed to reach rtu checkpoint for task \"%s\"\n",
					  __func__, trc->name);
				return 0;
			}

			Elf32_Ehdr *new_sw = (Elf32_Ehdr *)&_rtucontv3_elf_start;

			INFO_MSG("Starting runtime update.\n");
			if (!migrator_runtime_update(trc, new_sw)) {
				ERROR_MSG("Runtime updating failed.\n");
				return 0;
			}
			INFO_MSG("Runtime update complete. (-> v3)\n");
		} else {
			return 0;
		}

		vTaskDelay(20000/portTICK_RATE_MS);

		if ((trc = task_find("rtucont"))) {
			if (!task_wait_for_checkpoint(trc, cp_req_rtu)) {
				ERROR_MSG("%s: Failed to reach rtu checkpoint for task \"%s\"\n",
					  __func__, trc->name);
				return 0;
			}

			Elf32_Ehdr *new_sw = (Elf32_Ehdr *)&_rtucontv1_elf_start;

			INFO_MSG("Starting runtime update.\n");
			if (!migrator_runtime_update(trc, new_sw)) {
				ERROR_MSG("Runtime updating failed.\n");
				return 0;
			}
			INFO_MSG("Runtime update complete. (-> v1)\n");
		} else {
			return 0;
		}


	}
}
