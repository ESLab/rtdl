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

#define SYSTEM_MODULE CONTROLLER_V3

#include <System/system.h>
#include <System/pl011.h>

#include <App/rtucont/rtu_controller.h>
#include <App/rtu.h>

#include <stdio.h>

#define SET_VALUE (40.0)

#define P (-0.2)
#define I (-0.5)
#define D (0.0)

#define SIGNAL_MAX 500

#define PRBS_HIGH_VAL ( 1.0)
#define PRBS_LOW_VAL  (-1.0)

_RTU_DATA_ volatile float	integrator = 0.0;
_RTU_DATA_ volatile float	last_mv_u  = 0.0;

#define F1 (0.7905)
#define G0 (0.0770)
#define B0 (1.5553)

volatile int rtu_requested = 0;

void cpRequestHook(cp_req_t req_type)
{
	if (req_type == cp_req_rtu) {
		INFO_MSG("Controller run-time update request recieved.\n");
		rtu_requested = 1;
	}
	return;
}

static void print_data_row(int32_t input, int32_t output)
{
	int i;
	char ostr[40];

	sprintf(ostr, "%i, %i;\n", input, output);

	for (i = 0; ostr[i] != '\0'; i++) {
		xUARTSendCharacter(3, ostr[i], 0);
	}
}

int main()
{
	measurement_t	input;
	float		last_error = 0.0;

	while (!rtu_requested) {
		if (xQueueReceive(MeasurementQueue, (void *)&input, (portTickType)portMAX_DELAY) != pdPASS) {
			ERROR_MSG("Failed to receive measurement from plant.\n");
		} else {
			float fps = (float)input * (float)FRAME_COUNTING_FREQUENCY;
			float error = SET_VALUE - fps;

			integrator	   += error;
			float	difference  = error - last_error;
			last_error	    = error;

			float	fsignal	    = P*error + I*integrator + D*difference;
			float	mv_y	    = error;
			float	mv_u	    = -F1*last_mv_u + (G0/B0)*mv_y;

			fsignal		   += mv_u;

			int32_t		signal	= fsignal;
			controlsignal_t output	= signal < 0 ? 0 : (signal > SIGNAL_MAX ? SIGNAL_MAX : signal);

			print_data_row(error, mv_u);

			if (xQueueSendToBack(ControlSignalQueue, (void *)&output, (portTickType)portMAX_DELAY) != pdPASS) {
				ERROR_MSG("Failed to send control signal to plant.\n");
			}
		}
	}

	TASK_IN_SAFE_STATE();

	return 0;
}
