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

#include "task.h"
#include "queue.h"
#include "portable.h"

#include <System/types.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

/***********************
 *hardware measurements*
 ***********************/
#define SYS_CFGDATA	(*(volatile u_int32_t *)0x100000a0)
#define SYS_CFGCTRL	(*(volatile u_int32_t *)0x100000a4)
#define SYS_CFGSTAT	(*(volatile u_int32_t *)0x100000a8)

static u_int32_t ReadValue(int function,int site,int position,int device)
{
	int wait=10000;

	SYS_CFGSTAT=0;
	SYS_CFGCTRL=0x80000000|(function<<20)|(site<<16)|(position<<12)|device;
// Set up a transfer.

	while((SYS_CFGSTAT&0x01)==0 && wait--!=0)//; // Wait for completed flag.

	if(SYS_CFGSTAT&0x02) return 0xffffffff;

	return SYS_CFGDATA;
}

#if 0
static u_int32_t WriteValue(int function,int site,int position,int device, int data)
{

	taskENTER_CRITICAL();
	int wait=10000;
	SYS_CFGSTAT=0;
	SYS_CFGDATA = data;
	SYS_CFGCTRL=0x80000000| (1<<30) |(function<<20)|(site<<16)|(position<<12)|device;

	while((SYS_CFGSTAT&0x01)==0 && wait--!=0)
	{

	} // Wait for completed flag.
	vTaskDelay(100);
	taskEXIT_CRITICAL();
	
	if(SYS_CFGSTAT&0x02) return 0xffffffff;
	return 1;
}

static void PrintFrequency(int site,int position,int device)
{
	u_int32_t val=ReadValue(1,site,position,device);
	if(val==0xffffffff) printf("Error");
	else printf("%d.%06d MHz ",val/1000000,val%1000000);
}

static void PrintVoltage(int site,int position,int device)
{
	u_int32_t val=ReadValue(2,site,position,device);
	if(val==0xffffffff) printf("Error");
	else printf("%d.%06d V",val/1000000,val%1000000);
}

static void PrintCurrent(int site,int position,int device)
{
	u_int32_t val=ReadValue(3,site,position,device);
	if(val==0xffffffff) printf("Error");
	else printf("%d.%06d A",val/1000000,val%1000000);
}

static void PrintTemperature(int site,int position,int device)
{
	u_int32_t val=ReadValue(4,site,position,device);
	if(val==0xffffffff) printf("Error");
	else printf("%d.%06d C",val/1000000,val%1000000);
}

static void PrintPower(int site,int position,int device)
{
	u_int32_t val=ReadValue(12,site,position,device);
	if(val==0xffffffff) printf("Error");
	else {
		printf("%d.%06d W",val/1000000,val%1000000);
	}
}
#endif 

u_int32_t cortex_power_get()
{
	u_int32_t val=ReadValue(12,1,0,1);
	return val/10000;
}
