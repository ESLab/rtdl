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

#define SYSTEM_MODULE CONTROLLER_V1

#include <System/system.h>

#include <App/rtucont/rtu_controller.h>
#include <App/rtu.h>

#include <stdio.h>

#define SET_VALUE (40.0)

#define P (-0.2)
#define I (-0.5)
#define D (0.0)

#define SIGNAL_MAX 500

_RTU_DATA_ controller_state_v1 *state = NULL;

volatile int rtu_requested = 0;

void cpRequestHook(cp_req_t req_type)
{
	if (req_type == cp_req_rtu) {
		INFO_MSG("Controller run-time update request recieved.\n");
		rtu_requested = 1;
	}
	return;
}

static int setup_rtu_state( void )
{
	if (state == NULL) {
		INFO_MSG("Allocating state struct.\n");
		state		  = apptask_malloc(sizeof(controller_state_v1));
		if (state == NULL) {
			ERROR_MSG("Could not allocate state.\n");
			goto error;
		}
		state->ver	  = controller_version_1;
		state->integrator = 0.0;
	}

	switch (state->ver) {
	case controller_version_1:
		INFO_MSG("Found old state with version 1, ok.\n");
		break;
	case controller_version_2:
		INFO_MSG("Found old state with version 2, transforming.\n");
		{
			controller_state_v2	*old_state = (controller_state_v2 *)state;
			controller_state_v1	*new_state = apptask_malloc(sizeof(controller_state_v1));
			if (new_state == NULL) {
				ERROR_MSG("Could not allocate new state.\n");
				goto error;
			}
			new_state->ver	      = controller_version_1;
			new_state->integrator = old_state->integrator;
			apptask_free(state);
			state		      = new_state;
		}
		break;
	case controller_version_3:
		INFO_MSG("Found old state with version 3, transforming.\n");
		{
			controller_state_v3	*old_state = (controller_state_v3 *)state;
			controller_state_v1	*new_state = apptask_malloc(sizeof(controller_state_v1));
			if (new_state == NULL) {
				ERROR_MSG("Could not allocate new state.\n");
				goto error;
			}
			new_state->ver	      = controller_version_1;
			new_state->integrator = old_state->integrator;
			apptask_free(state);
			state		      = new_state;
		}
		break;
	}

	return 1;

error:
	return 0;
}

int main()
{
	measurement_t	input;
	float		last_error = 0.0;

	if (!setup_rtu_state()) {
		ERROR_MSG("Could not setup rtu state.\n");
		goto error;
	}

	while (!rtu_requested) {
		if (xQueueReceive(MeasurementQueue, (void *)&input, (portTickType)portMAX_DELAY) != pdPASS) {
			ERROR_MSG("Failed to receive measurement from plant.\n");
		} else {
			float		fps	    = (float)input * (float)FRAME_COUNTING_FREQUENCY;
			float		error	    = SET_VALUE - fps;

			state->integrator	   += error;
			float		difference  = error - last_error;
			last_error		    = error;

			float		fsignal	    = P*error + I*state->integrator + D*difference;
			int32_t		signal	    = fsignal;
			controlsignal_t output	    = signal < 0 ? 0 : (signal > SIGNAL_MAX ? SIGNAL_MAX : signal);

			if (xQueueSendToBack(ControlSignalQueue, (void *)&output, (portTickType)portMAX_DELAY) != pdPASS) {
				ERROR_MSG("Failed to send control signal to plant.\n");
			}
		}
	}

	TASK_IN_SAFE_STATE();
	ERROR_MSG("Task got out of state state, entering infinite loop.\n");

	while (1)
		;
	return 0;

error:
	ERROR_MSG("Task in error state, entering infinite loop.\n");
	while (1)
		;

	return 0;
}
