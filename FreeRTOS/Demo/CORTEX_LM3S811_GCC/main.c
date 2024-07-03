/* Environment includes. */
#include "DriverLib.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define mainSENSOR_TASK_PRIORITY (tskIDLE_PRIORITY + 3)

/*
 * Configure the processor and peripherals for this demo.
 */
static void prvSetupHardware( void );
static void vPrintTask( void *pvParameters );

QueueHandle_t xPrintQueue;

int main( void )
{
	/* Configure the clocks, UART and GPIO. */
	prvSetupHardware();

	/* Create the queue used to pass message to vPrintTask. */
	xPrintQueue = xQueueCreate( 10, sizeof( char * ) );

	/* Start the tasks defined within the file. */
	//xTaskCreate( vCheckTask, "Check", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY, NULL );
	xTaskCreate( vPrintTask, "Print", configMINIMAL_STACK_SIZE, NULL, mainSENSOR_TASK_PRIORITY - 1, NULL );

	/* Start the scheduler. */
	vTaskStartScheduler();
	
	/* Will only get here if there was insufficient heap to start the
	scheduler. */

	return 0;
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
	/* Setup the PLL. */
	SysCtlClockSet( SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_6MHZ );

	/* Initialise the LCD> */
    OSRAMInit( false );
    OSRAMStringDraw("www.FreeRTOS.org", 0, 0);
	OSRAMStringDraw("LM3S811 demo", 16, 1);
}
/*-----------------------------------------------------------*/

static void vPrintTask( void *pvParameters )
{
char *pcMessage;
unsigned portBASE_TYPE uxLine = 0, uxRow = 0;

	for( ;; )
	{
		/* Wait for a message to arrive. */
		xQueueReceive( xPrintQueue, &pcMessage, portMAX_DELAY );

		/* Write the message to the LCD. */
		uxRow++;
		uxLine++;
		OSRAMClear();
		OSRAMStringDraw( pcMessage, uxLine & 0x3f, uxRow & 0x01);
	}
}

void vGPIO_ISR(void)
{
    // Implementación de la rutina de interrupción del GPIO
}

void vUART_ISR(void)
{
    // Implementación de la rutina de interrupción del UART
}
