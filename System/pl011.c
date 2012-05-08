/*
    FreeRTOS V7.0.1 - Copyright (C) 2011 Real Time Engineers Ltd.


	FreeRTOS supports many tools and architectures. V7.0.0 is sponsored by:
	Atollic AB - Atollic provides professional embedded systems development
	tools for C/C++ development, code analysis and test automation.
	See http://www.atollic.com


    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

/* UART Driver for the PL011 Peripheral. */

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include <semphr.h>

#include <pl011.h>
/*----------------------------------------------------------------------------*/

#define UART_USE_INTERRUPT			0

#define UART0_BASE			( 0x10009000UL )		/* Realview PBX Cortex-A9, Versatile Express. */
#define UART1_BASE			( 0x1000A000UL )		/* Realview PBX Cortex-A9, Versatile Express. */
#define UART2_BASE			( 0x1000B000UL )		/* Versatile Express. */
#define UART3_BASE			( 0x1000C000UL )		/* Versatile Express. */
#define UART_DEVICE_TYPE	0

#define UARTDR(x)			( (volatile unsigned char *)	( (x) + 0x0000UL ) )	/* Data Register */
#define UARTRSR_UARTECR(x)	( (volatile unsigned char *)	( (x) + 0x0004UL ) )	/* Receive Status Register / Error Clear Register */
#define UARTFR(x)			( (volatile unsigned short *)( (x) + 0x0018UL ) )	/* Flag Register */
#define UARTILPR(x)			( (volatile unsigned char *)	( (x) + 0x0020UL ) )	/* IrDA Low-Power Counter Register */
#define UARTIBRD(x)			( (volatile unsigned short *)( (x) + 0x0024UL ) )	/* Integer Baud Rate Register */
#define UARTFBRD(x)			( (volatile unsigned char *)	( (x) + 0x0028UL ) )	/* Fractional Baud Rate Register */
#define UARTLCR_H(x)		( (volatile unsigned char *)	( (x) + 0x002CUL ) )	/* Line Control Register */
#define UARTCR(x)			( (volatile unsigned short *)( (x) + 0x0030UL ) )	/* Control Register */
#define UARTIFLS(x)			( (volatile unsigned char *)	( (x) + 0x0034UL ) )	/* Interrupt FIFO Level Select Register */
#define UARTIMSC(x)			( (volatile unsigned short *)( (x) + 0x0038UL ) )	/* Interrupt Mask Set/Clear Register */
#define UARTRIS(x)			( (volatile unsigned short *)( (x) + 0x003CUL ) )	/* Raw Interrupt Status Register */
#define UARTMIS(x)			( (volatile unsigned short *)( (x) + 0x0040UL ) )	/* Masked Interrupt Status Register */
#define UARTICR(x)			( (volatile unsigned short *)( (x) + 0x0044UL ) )	/* Interrupt Clear Register */
#define UARTDMACR(x)		( (volatile unsigned char *)	( (x) + 0x0048UL ) )	/* DMA Control Register */
#define UARTPeriphID0(x)	( (volatile unsigned char *)	( (x) + 0x0FE0UL ) )	/* UARTPeriphID0 Register */
#define UARTPeriphID1(x)	( (volatile unsigned char *)	( (x) + 0x0FE4UL ) )	/* UARTPeriphID1 Register */
#define UARTPeriphID2(x)	( (volatile unsigned char *)	( (x) + 0x0FE8UL ) )	/* UARTPeriphID2 Register */
#define UARTPeriphID3(x)	( (volatile unsigned char *)	( (x) + 0x0FECUL ) )	/* UARTPeriphID3 Register */
#define UARTPCellID0(x)		( (volatile unsigned char *)	( (x) + 0x0FF0UL ) )	/* UARTPCellID0 Register */
#define UARTPCellID1(x)		( (volatile unsigned char *)	( (x) + 0x0FF4UL ) )	/* UARTPCellID1 Register */
#define UARTPCellID2(x)		( (volatile unsigned char *)	( (x) + 0x0FF8UL ) )	/* UARTPCellID2 Register */
#define UARTPCellID3(x)		( (volatile unsigned char *)	( (x) + 0x0FFCUL ) )	/* UARTPCellID3 Register */

#define UART_INT_STATUS_TX	( 1 << 5 )
#define UART_INT_STATUS_RX	( 1 << 4 )

#define UART_FLAG_TXFE		( 1 << 7 )
#define UART_FLAG_TXFF		( 1 << 5 )
#define UART_FLAG_RXFE		( 1 << 4 )

#define UART_CLK_HZ				( 3686400UL )

#define UART_FIFO_SIZE_BYTES	( 32UL )
#define UART0_VECTOR_ID			( 44 )
#define UART1_VECTOR_ID			( 45 )
#define UART2_VECTOR_ID			( 46 )
#define UART3_VECTOR_ID			( 47 )
#define UART4_VECTOR_ID			( 48 )

#define TX_QUEUE				( 0 )
#define RX_QUEUE				( 1 )
/*----------------------------------------------------------------------------*/

#if configPLATFORM == 0 || configPLATFORM == 2

#if configPLATFORM == 0
#define UART_CLK 3686400 /* May be wrong? */
#elif configPLATFORM == 2
#define UART_CLK 24000000
#endif

#if UART_USE_INTERRUPT

static xQueueHandle xUartQueues[5][2] = { { NULL, NULL } };
/*----------------------------------------------------------------------------*/

void vUARTInterruptHandler( void *pvBaseAddress )
{
unsigned long ulBase = (unsigned long)pvBaseAddress;
unsigned short usStatus = 0;
signed char cTransmitChar = 0;
signed char cReceiveChar = 0;
portBASE_TYPE xTaskWoken = pdFALSE;
xQueueHandle xTxQueue = NULL;
xQueueHandle xRxQueue = NULL;
unsigned long ulUART = 0;

	/* Select the UART Queues. */
	switch ( ulBase )
	{
	case UART0_BASE:
		ulUART = 0;
		break;
	case UART1_BASE:
		ulUART = 1;
		break;
	case UART2_BASE:
		ulUART = 2;
		break;
	case UART3_BASE:
		ulUART = 3;
		break;
	}

	xTxQueue = xUartQueues[ulUART][TX_QUEUE];
	xRxQueue = xUartQueues[ulUART][RX_QUEUE];

	/* Figure out the reason for the interrupt. */
	usStatus = *UARTMIS(ulBase);

	if ( usStatus & UART_INT_STATUS_TX )
	{
		/* Buffer is almost empty, try to refill. */
		while ( *UARTFR(ulBase) & UART_FLAG_TXFF )
		{
			if ( pdTRUE == xQueueReceiveFromISR( xTxQueue, &cTransmitChar, &xTaskWoken ) )
			{
				*UARTDR(ulBase) = cTransmitChar;
			}
			else
			{
				/* Run out of characters to send. */
				break;
			}
		}
	}

	if ( usStatus & UART_INT_STATUS_RX )
	{
		/* Receive Buffer is almost full. */
		while ( !( *UARTFR(ulBase) & UART_FLAG_RXFE ) )
		{
			cReceiveChar = *UARTDR(ulBase);
			if ( pdTRUE != xQueueSendFromISR( xRxQueue, &cReceiveChar, &xTaskWoken ) )
			{
				/* Receive Queue is Full. */
				/* Not good as we are going to lose this character. */
				break;
			}
		}
	}

	/* Here we should deal with any errors. */

	/* Acknownledge the interrupt. */
	*UARTICR(ulBase) = usStatus;

	/* Finally, switch task if necessary. */
	portEND_SWITCHING_ISR(xTaskWoken);
}
/*----------------------------------------------------------------------------*/

#endif /* UART_USE_INTERRUPT */

void vUARTInitialise(unsigned long ulUARTPeripheral, unsigned long ulBaud, unsigned long ulQueueSize )
{
extern void vPortInstallInterruptHandler( void (*vHandler)(void *), void *pvParameter, unsigned long ulVector, unsigned char ucEdgeTriggered, unsigned char ucPriority, unsigned char ucProcessorTargets );
unsigned long ulBase = 0;
#if UART_USE_INTERRUPT
unsigned long ulVectorID = 0;
#endif

	switch ( ulUARTPeripheral )
	{
	case 0:
		ulBase = UART0_BASE;
		#if UART_USE_INTERRUPT
		ulVectorID = UART0_VECTOR_ID;
		/* Create the Queues to Handle the bytes. */
		xUartQueues[0][TX_QUEUE] = xSemaphoreCreateCounting( ulQueueSize, ulQueueSize );
		xUartQueues[0][RX_QUEUE] = xQueueCreate( ulQueueSize, sizeof( char ) );
		#endif
		break;
	case 1:
		ulBase = UART1_BASE;
		#if UART_USE_INTERRUPT
		ulVectorID = UART1_VECTOR_ID;
		/* Create the Queues to Handle the bytes. */
		xUartQueues[1][TX_QUEUE] = xQueueCreate( ulQueueSize, sizeof( char ) );
		xUartQueues[1][RX_QUEUE] = xQueueCreate( ulQueueSize, sizeof( char ) );
		#endif
		break;
	case 2:
		ulBase = UART2_BASE;
		#if UART_USE_INTERRUPT
		ulVectorID = UART2_VECTOR_ID;
		/* Create the Queues to Handle the bytes. */
		xUartQueues[2][TX_QUEUE] = xQueueCreate( ulQueueSize, sizeof( char ) );
		xUartQueues[2][RX_QUEUE] = xQueueCreate( ulQueueSize, sizeof( char ) );
		#endif
		break;
	case 3:
		ulBase = UART3_BASE;
		#if UART_USE_INTERRUPT
		ulVectorID = UART3_VECTOR_ID;
		/* Create the Queues to Handle the bytes. */
		xUartQueues[3][TX_QUEUE] = xQueueCreate( ulQueueSize, sizeof( char ) );
		xUartQueues[3][RX_QUEUE] = xQueueCreate( ulQueueSize, sizeof( char ) );
		#endif
		break;
	}

	/* First Disable the Peripheral. */
	*UARTCR(ulBase) = 0UL;

	/* Configure the Peripheral. */
	*UARTIBRD(ulBase) = UART_CLK / ( 16 * ulBaud );
	*UARTFBRD(ulBase) = UART_CLK % ( 16 * ulBaud );
	*UARTLCR_H(ulBase) = ( 3 << 5 ) | ( 1 << 4 );

	/* Configure the Interrupts. */
	*UARTIFLS(ulBase) = ( 0 << 3 ) | ( 0 << 0 );
	*UARTICR(ulBase) = 0xFFFF;	/* Clear all Iterrupts. */

#if UART_USE_INTERRUPT
	*UARTIMSC(ulBase) = UART_INT_STATUS_TX | UART_INT_STATUS_RX;
	vPortInstallInterruptHandler( vUARTInterruptHandler, (void *)ulBase, ulVectorID, pdFALSE, configMAX_SYSCALL_INTERRUPT_PRIORITY, 1 );
#endif /* UART_USE_INTERRUPT */

	/* Finally enable the peripheral. */
	*UARTCR(ulBase) = ( 1 << 9 ) | ( 1 << 8 ) | /* Loopback Enable ( 1 << 7 ) | */ ( 1 << 0 );
}
/*----------------------------------------------------------------------------*/

portBASE_TYPE xUARTSendCharacter( unsigned long ulUARTPeripheral, signed char cChar, portTickType xDelay )
{
unsigned long ulBase = 0;
portBASE_TYPE xReturn = pdFALSE;
#if UART_USE_INTERRUPT
xQueueHandle xTxQueue = NULL;
unsigned char ucStatus = 0;
#endif	/* UART_USE_INTERRUPT */

	switch ( ulUARTPeripheral )
	{
	case 0:
		ulBase = UART0_BASE;
		#if UART_USE_INTERRUPT
		xTxQueue = xUartQueues[0][TX_QUEUE];
		#endif
		break;
	case 1:
		ulBase = UART1_BASE;
		#if UART_USE_INTERRUPT
		xTxQueue = xUartQueues[1][TX_QUEUE];
		#endif
		break;
	case 2:
		ulBase = UART2_BASE;
		#if UART_USE_INTERRUPT
		xTxQueue = xUartQueues[2][TX_QUEUE];
		#endif
		break;
	case 3:
		ulBase = UART3_BASE;
		#if UART_USE_INTERRUPT
		xTxQueue = xUartQueues[3][TX_QUEUE];
		#endif
		break;
	}

	if ( 0 != ulBase )
	{
#if UART_USE_INTERRUPT
		xReturn = xQueueSend( xTxQueue, &cChar, xDelay );
		taskENTER_CRITICAL();
			ucStatus = *UARTFR(ulBase);
		taskEXIT_CRITICAL();

		if ( ucStatus & UART_FLAG_TXFE )
		{
			/* Need to kick of the Tx. */
			(void)xQueueReceive( xTxQueue, &cChar, 0 );
			*UARTDR(ulBase) = cChar;
			xReturn = pdTRUE;
		}
#else
		while ( !(*UARTFR(ulBase) & UART_FLAG_TXFE) );
		*UARTDR(ulBase) = cChar;
		xReturn = pdTRUE;
#endif	/* UART_USE_INTERRUPT */
	}
	return xReturn;
}
/*----------------------------------------------------------------------------*/

portBASE_TYPE xUARTReceiveCharacter( unsigned long ulUARTPeripheral, signed char *pcChar, portTickType xDelay )
{
unsigned long ulBase = 0;
portBASE_TYPE xReturn = pdFALSE;
#if UART_USE_INTERRUPT
xQueueHandle xRxQueue = NULL;
#endif

	switch ( ulUARTPeripheral )
	{
	case 0:
		ulBase = UART0_BASE;
		#if UART_USE_INTERRUPT
		xRxQueue = xUartQueues[0][RX_QUEUE];
		#endif
		break;
	case 1:
		ulBase = UART1_BASE;
		#if UART_USE_INTERRUPT
		xRxQueue = xUartQueues[1][RX_QUEUE];
		#endif
		break;
	case 2:
		ulBase = UART2_BASE;
		#if UART_USE_INTERRUPT
		xRxQueue = xUartQueues[2][RX_QUEUE];
		#endif
		break;
	case 3:
		ulBase = UART3_BASE;
		#if UART_USE_INTERRUPT
		xRxQueue = xUartQueues[3][RX_QUEUE];
		#endif
		break;
	}

	if ( 0 != ulBase )
	{
#if UART_USE_INTERRUPT
		xReturn = xQueueReceive( xRxQueue, pcChar, xDelay );
#else
		if ( ( *UARTFR(ulBase) & UART_FLAG_RXFE ) == 0 )
		{
			*pcChar = *UARTDR(ulBase);
			xReturn = pdTRUE;
		}
#endif /* UART_USE_INTERRUPT */
	}

	return xReturn;
}
/*----------------------------------------------------------------------------*/

#endif /* configPLATFORM == 0 */

