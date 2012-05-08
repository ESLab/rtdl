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

#include <task.h>
#include <queue.h>

#include <stdio.h>
#include <string.h>

#include <pl011.h>
#include <serial.h>

static void prvSetupHardware( void );

#define mainPRINT_PORT				( configUART_PORT )
#if configPLATFORM == 2
#define mainPRINT_BAUDRATE			( 38400 )
#else
#define mainPRINT_BAUDRATE			( 115200 )
#endif

int main()
{
	char tmp_str[32];

	//portDISABLE_INTERRUPTS();

	//vUARTInitialise( mainPRINT_PORT, mainPRINT_BAUDRATE, 64 );

	prvSetupHardware();

	sprintf(tmp_str, "Hello World\r\n");
	vSerialPutString((xComPortHandle)configUART_PORT, (const signed char * const)tmp_str, strlen(tmp_str));

	//printf("jaha asd\r\n");

	vTaskStartScheduler();
	
	return 0;
}

#if 1

/*-----------------------------------------------------------*/

#if configPLATFORM == 2

static unsigned long prvReadP15_C1_C0_0( void )
{
volatile unsigned long ulReturn = 0UL;
	/* Read Configuration Register C15, c0, 0. */
	__asm volatile(
			" mrc p15, 0, %[retval], c1, c0, 0 			\n"
			: [retval] "=r" (ulReturn) : :
			);
	return ulReturn;
}
/*----------------------------------------------------------------------------*/

#if 0
static void prvWriteP15_C1_C0_0( unsigned long ulValue )
{
	/* Write Configuration Register C15, c0, 0. */
	__asm volatile(
			" mcr p15, 0, %[vbar], c1, c0, 0 			\n"
			: : [vbar] "r" (ulValue) :
			);
}
#endif
/*----------------------------------------------------------------------------*/

static unsigned long prvReadP15_C1_C0_1( void )
{
volatile unsigned long ulReturn = 0UL;
	/* Read Configuration Register C15, c0, 0. */
	__asm volatile(
			" mrc p15, 0, %[retval], c1, c0, 1 			\n"
			: [retval] "=r" (ulReturn) : :
			);
	return ulReturn;
}
/*----------------------------------------------------------------------------*/

static unsigned long prvReadP15_C1_C0_2( void )
{
volatile unsigned long ulReturn = 0UL;
	/* Read Configuration Register C15, c0, 0. */
	__asm volatile(
			" mrc p15, 0, %[retval], c1, c0, 2 			\n"
			: [retval] "=r" (ulReturn) : :
			);
	return ulReturn;
}
/*----------------------------------------------------------------------------*/

static unsigned long prvReadP15_C1_C1_0( void )
{
volatile unsigned long ulReturn = 0UL;
	/* Read Configuration Register C15, c0, 0. */
	__asm volatile(
			" mrc p15, 0, %[retval], c1, c1, 0 			\n"
			: [retval] "=r" (ulReturn) : :
			);
	return ulReturn;
}
/*----------------------------------------------------------------------------*/

#if 0
static void prvWriteP15_C1_C1_0( unsigned long ulValue )
{
	/* Write Configuration Register C15, c0, 0. */
	__asm volatile(
			" mcr p15, 0, %[val], c1, c1, 0 			\n"
			: : [val] "r" (ulValue) :
			);
}
#endif
/*----------------------------------------------------------------------------*/

static unsigned long prvReadP15_C1_C1_1( void )
{
volatile unsigned long ulReturn = 0UL;
	/* Read Configuration Register C15, c0, 0. */
	__asm volatile(
			" mrc p15, 0, %[retval], c1, c1, 1 			\n"
			: [retval] "=r" (ulReturn) : :
			);
	return ulReturn;
}
/*----------------------------------------------------------------------------*/

static unsigned long prvReadP15_C1_C1_2( void )
{
volatile unsigned long ulReturn = 0UL;
	/* Read Configuration Register C15, c0, 0. */
	__asm volatile(
			" mrc p15, 0, %[retval], c1, c1, 2 			\n"
			: [retval] "=r" (ulReturn) : :
			);
	return ulReturn;
}

/*----------------------------------------------------------------------------*/

static unsigned long prvReadP15_C12_C0_1( void )
{
volatile unsigned long ulReturn = 0UL;
	/* Read Configuration Register C15, c0, 0. */
	__asm volatile(
			" mrc p15, 0, %[retval], c12, c0, 1 		\n"
			: [retval] "=r" (ulReturn) : :
			);
	return ulReturn;
}
/*----------------------------------------------------------------------------*/

#if 0
static void prvWriteP15_C12_C0_1( unsigned long ulValue )
{
	/* Write Configuration Register C15, c0, 0. */
	__asm volatile(
			" mcr p15, 0, %[mvbar], c12, c0, 1 			\n"
			: : [mvbar] "r" (ulValue) :
			);
}
#endif
/*----------------------------------------------------------------------------*/
#endif /* configPLATFORM == 2 */

static void prvSetupHardware( void )
{
	unsigned long ulVector = 0UL;
	unsigned long ulValue = 0UL;
	char cAddress[32];

	portDISABLE_INTERRUPTS();

	/* Install the Spurious Interrupt Handler to help catch interrupts. */
	extern void vPortUnknownInterruptHandler( void *pvParameter );
	extern void vPortInstallInterruptHandler( void (*vHandler)(void *), void *pvParameter, unsigned long ulVector, unsigned char ucEdgeTriggered, unsigned char ucPriority, unsigned char ucProcessorTargets );
	for ( ulVector = 0; ulVector < portMAX_VECTORS; ulVector++ )
		vPortInstallInterruptHandler( vPortUnknownInterruptHandler, (void *)ulVector, ulVector, pdTRUE, configMAX_SYSCALL_INTERRUPT_PRIORITY, 1 );

	extern void vUARTInitialise(unsigned long ulUARTPeripheral, unsigned long ulBaud, unsigned long ulQueueSize );
	vUARTInitialise( mainPRINT_PORT, mainPRINT_BAUDRATE, 64 );

	ulValue = portCORE_ID();
	sprintf( cAddress, "Core: %ld\r\n", ulValue );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );

	ulValue = (unsigned long)main;
	sprintf( cAddress, "Main: 0x%08lX\r\n", ulValue );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );

	/* Read the Configuration Registers to find the vital information. */
	ulValue = prvReadP15_C1_C0_0();
	sprintf( cAddress, "SCTLR: 0x%08lX\r\n", ulValue );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );

	/* Read the Configuration Registers to find the vital information. */
	ulValue = prvReadP15_C1_C0_1();
	sprintf( cAddress, "ACTLR: 0x%08lX\r\n", ulValue );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );

	/* Read the Configuration Registers to find the vital information. */
	ulValue = prvReadP15_C1_C0_2();
	sprintf( cAddress, "CPACR: 0x%08lX\r\n", ulValue );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );

	/* Read the Configuration Registers to find the vital information. */
	ulValue = prvReadP15_C1_C1_0();
	sprintf( cAddress, "SCR: 0x%08lX\r\n", ulValue );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );

	/* Read the Configuration Registers to find the vital information. */
	ulValue = prvReadP15_C1_C1_1();
	sprintf( cAddress, "SDER: 0x%08lX\r\n", ulValue );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );

	/* Read the Configuration Registers to find the vital information. */
	ulValue = prvReadP15_C1_C1_2();
	sprintf( cAddress, "NSACR: 0x%08lX\r\n", ulValue );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );

	/* Read the Configuration Registers to find the vital information. */
	ulValue = prvReadP15_C12_C0_1();
	sprintf( cAddress, "MVBAR: 0x%08lX\r\n", ulValue );
	vSerialPutString((xComPortHandle)configUART_PORT,(const signed char * const)cAddress, strlen(cAddress) );

	/* Perform any other peripheral configuration. */
}
#endif

void vApplicationMallocFailedHook( void )
{
	__asm volatile (" smc #0 ");
}

