/* Environment includes. */
#include "DriverLib.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "hw_memmap.h"
#include "portable.h"
#include "uart.h"

#define OLED_WIDTH 96
#define OLED_HEIGHT 16
#define MAX_FILTER_SIZE 50

/* Delay between cycles of the 'GENERATOR' task. */
#define mainGENERATOR_DELAY ((TickType_t)100 / portTICK_PERIOD_MS)

#define mainSENSOR_TASK_PRIORITY (tskIDLE_PRIORITY + 3)

/*
 * Configure the processor and peripherals for this demo.
 */
static void prvSetupHardware( void );
static void vNumberGeneratorTask(void *pvParameters);
static void vDisplayTask(void *pvParameters);
void intToStr(int num, char *str);

QueueHandle_t xPrintQueue;

int main( void )
{
	/* Configure the clocks, UART and GPIO. */
	prvSetupHardware();

	/* Create the queue used to pass message to vPrintTask. */
	xPrintQueue = xQueueCreate( 10, sizeof(int) );

	/* Start the tasks defined within the file. */
    xTaskCreate(vNumberGeneratorTask, "NumberGen", configMINIMAL_STACK_SIZE, NULL, mainSENSOR_TASK_PRIORITY - 1, NULL);
    xTaskCreate(vDisplayTask, "Display", configMINIMAL_STACK_SIZE, NULL, mainSENSOR_TASK_PRIORITY - 2, NULL);

	/* Start the scheduler. */
	vTaskStartScheduler();

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

static void vNumberGeneratorTask(void *pvParameters)
{
    int number = 0;
	TickType_t xLastExecutionTime;

	/* Initialise xLastExecutionTime so the first call to vTaskDelayUntil() works
	* correctly. */
	xLastExecutionTime = xTaskGetTickCount();

    for (;;)
    {	
		vTaskDelayUntil(&xLastExecutionTime, mainGENERATOR_DELAY);

        /* Send the number to the queue. */
        xQueueSend(xPrintQueue, &number, portMAX_DELAY);

        /* Increment the number. */
        number = (number + 1) % 41;

        /* Delay for 1 second. */
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void vDisplayTask(void *pvParameters)
{
	int value = 0;
	char displayMessage[16];
	OSRAMClear();

	for (;;) {
		/* Wait for a message to arrive. */
		xQueueReceive(xPrintQueue, &value, portMAX_DELAY);
		OSRAMClear();
		intToStr(value, displayMessage);
		OSRAMStringDraw("El valor es:", 0, 0);
		OSRAMStringDraw(displayMessage, 16, 1);
		/* Delay for 1 second. */
		vTaskDelay(pdMS_TO_TICKS(1000));


	}

}

void vGPIO_ISR(void)
{
    // Código de manejo de la interrupción GPIO
}

void vUART_ISR(void)
{
    // Código de manejo de la interrupción UART
}

void intToStr(int num, char *str) {
    int i = 0;
    int isNegative = 0;

    /* Handle 0 explicitly, otherwise empty string is printed for 0 */
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // Handle negative numbers
    if (num < 0) {
        isNegative = 1;
        num = -num;
    }

    // Process individual digits
    while (num != 0) {
        int rem = num % 10;
        str[i++] = rem + '0';
        num = num / 10;
    }

    // If the number is negative, append '-'
    if (isNegative) {
        str[i++] = '-';
    }

    str[i] = '\0'; // Append string terminator

    // Reverse the string
    for (int start = 0, end = i - 1; start < end; start++, end--) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
    }
}

