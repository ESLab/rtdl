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

#include <System/arch/arm9_novm/usart/freertos_if.h>

static AT91S_USART *get_usart(unsigned long ulUARTPeripheral)
{
	AT91S_USART *usart;

	switch (ulUARTPeripheral) {
	case 0:
		usart = AT91C_BASE_US0;
		break;
	case 1:
		usart = AT91C_BASE_US1;
		break;
	case 2:
		usart = AT91C_BASE_US2;
		break;
	default:
		usart = NULL;
		break;
	}

	return usart;
}

void vUARTInitialise(unsigned long ulUARTPeripheral, unsigned long ulBaud, unsigned long ulQueueSize )
{
	AT91S_USART *usart = get_usart(ulUARTPeripheral);

	if (usart != NULL) 
		USART_Configure(usart, USART_MODE_ASYNCHRONOUS, ulBaud, 100*1000*1000);
}

portBASE_TYPE	xUARTReceiveCharacter( unsigned long ulUARTPeripheral, signed char *pcChar, portTickType xDelay )
{
	portBASE_TYPE xReturn = pdFALSE;
	AT91S_USART *usart = get_usart(ulUARTPeripheral);

	if (usart == NULL)
		return xReturn;

	*pcChar = USART_Read(usart, xDelay);
	xReturn = pdTRUE;

	return xReturn;
}

portBASE_TYPE xUARTSendCharacter( unsigned long ulUARTPeripheral, signed char cChar, portTickType xDelay )
{
	portBASE_TYPE xReturn = pdFALSE;
	AT91S_USART *usart = get_usart(ulUARTPeripheral);

	if (usart == NULL)
		return xReturn;

	USART_Write(usart, cChar, xDelay);
	xReturn = pdTRUE;

	return xReturn;
}
