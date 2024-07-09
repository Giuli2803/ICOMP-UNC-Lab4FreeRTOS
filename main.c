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
#include "hw_types.h"
#include "sysctl.h"
#include "interrupt.h"
#include "hw_ints.h"

/* Defines */
#define OLED_WIDTH 96
#define OLED_HEIGHT 16
#define MAX_FILTER_SIZE 50
#define mainGENERATOR_DELAY ((TickType_t)100 / portTICK_PERIOD_MS)  // Tiempo de espera entre los ciclos del 'GENERATOR' task. (100 ms)
#define mainSENSOR_TASK_PRIORITY (tskIDLE_PRIORITY + 3)             // Prioridad de la tarea 'GENERATOR'.
#define mainFILTER_TASK_PRIORITY (tskIDLE_PRIORITY + 2)             // Prioridad de la tarea 'FILTER'.
#define mainDISPLAY_TASK_PRIORITY (tskIDLE_PRIORITY + 2)            // Prioridad de la tarea 'DISPLAY'.
#define mainSTATS_TASK_PRIORITY (tskIDLE_PRIORITY + 1)              // Prioridad de la tarea 'STATS'.

/* Configuracion UART - note que no utiliza FIFO por lo que no es muy eficiente. */
#define mainBAUD_RATE (19200)

/* Declaración de funciones */
static void prvSetupHardware( void );
static void vNumberGeneratorTask(void *pvParameters);   //Task 1
static void vFilterTask(void *pvParameters);            //Task 2
static void vDisplayTask(void *pvParameters);           // Task 3
static void vStatsTask(void *pvParameters);             //Task 4
void addValueToSignal(unsigned char image[OLED_WIDTH * 2], int value);
void intToStr(int num, char *str);                      // Función para convertir un entero a un string
void vUART_ISR(void);

/* Defino colas de mensajes para el envio de datos */
QueueHandle_t xSensorQueue;
QueueHandle_t xDisplayQueue;
QueueHandle_t xFilterQueue;

/* Valor del filtro */
int filterSize = 10; 

int main( void )
{
    /* Configuro los clocks, UART y el Display. */
    prvSetupHardware();

    /* Instancio las colas de mensajes */
    //xSensorQueue = xQueueCreate(10, sizeof(int));
    xFilterQueue = xQueueCreate(10, sizeof(int));
    xDisplayQueue = xQueueCreate(10,sizeof(int));

	/* Defino las tareas solicitadas */
    xTaskCreate(vNumberGeneratorTask, "NumberGen", configMINIMAL_STACK_SIZE, NULL, mainSENSOR_TASK_PRIORITY, NULL);
    //xTaskCreate(vFilterTask, "Filter", configMINIMAL_STACK_SIZE, NULL, mainFILTER_TASK_PRIORITY, NULL);
    xTaskCreate(vDisplayTask, "Display", configMINIMAL_STACK_SIZE, NULL, mainDISPLAY_TASK_PRIORITY, NULL);
    //xTaskCreate(vStatsTask, "Stats", configMINIMAL_STACK_SIZE, NULL, mainSTATS_TASK_PRIORITY, NULL);

	/* inicio el scheduler. */
	vTaskStartScheduler();

	return 0;
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
	/* Setup the PLL. */
	SysCtlClockSet( SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_6MHZ );

	/* Initialise the LCD */
    OSRAMInit( false );
    OSRAMStringDraw("www.FreeRTOS.org", 0, 0);
	OSRAMStringDraw("LM3S811 demo", 16, 1);

    /* Enable the UART. */
    //SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    /* Configure the UART for 8-N-1 operation. */
    //UARTConfigSet(UART0_BASE, mainBAUD_RATE, UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE);
}
/*-----------------------------------------------------------*/

static void vNumberGeneratorTask(void *pvParameters)
{
    int number = 0;
	TickType_t xLastExecutionTime;

	/* Inicializa la variable xLastExecutionTime con el valor actual de ticks.*/
	xLastExecutionTime = xTaskGetTickCount();

    for (;;)
    {	
		vTaskDelayUntil(&xLastExecutionTime, mainGENERATOR_DELAY); // Define el periodo de ejecución de la tarea

        /* Envio numero por la Cola de mensajes */
        
        xQueueSend(xDisplayQueue, &number, portMAX_DELAY);
        //xQueueSend(xFilterQueue, &number, portMAX_DELAY);

        /* Incrementa el numero que simula el sensor. */
        number = (number + 1) % 16;

    }
}

static void vFilterTask(void *pvParameters)
{
    int sensorValue = 0;
    int filterBuffer[MAX_FILTER_SIZE] = {0};
    int index = 0;
    int sum = 0;

    for (;;)
    {
        if (xQueueReceive(xFilterQueue, &sensorValue, portMAX_DELAY))
        {
            /*sum -= filterBuffer[index];
            filterBuffer[index] = sensorValue;
            sum += sensorValue;
            index = (index + 1) % filterSize;

            int filteredValue = sum / filterSize;
            */

            //xQueueSend(xFilterQueue, &filteredValue, portMAX_DELAY);
        }
    }
}

static void vDisplayTask(void *pvParameters)
{   
    static unsigned char signal[OLED_WIDTH * 2] = {0};
    int value = 0;

    OSRAMClear();
    addValueToSignal(signal, 0);
    OSRAMImageDraw(signal, 0, 0, OLED_WIDTH, 2);

    for (;;) {
        /* Wait for a message to arrive. */
        xQueueReceive(xDisplayQueue, &value, portMAX_DELAY);

        /* Write the image to the LCD. */
        addValueToSignal(signal, value);
        OSRAMImageDraw(signal, 0, 0, OLED_WIDTH, 2);
    }
}

static void vStatsTask(void *pvParameters)
{
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(5000)); // Muestra estadísticas cada 5 segundos
        // Mostrar estadísticas de las tareas (uso de CPU, memoria, etc.)
    }
}

/**
 * @brief Adds a value to the OLED signal array and shifts the existing values.
 * @param image The signal array.
 * @param value The value to add.
 */
void addValueToSignal(unsigned char image[OLED_WIDTH * 2], int value) 
{
    // shift signal        [Arreglo de arriba con valores hasta 8] [Arreglo de abajo con valores hasta 8]
    // [OLED_WIDTH * 2] = [(01234567),(89101112131415), (...95),      .....191]
    for (int i = OLED_WIDTH - 1; i > 0; i--) { // arranca desde 95 hasta 1
        //como son char muevo de un byte en cada movimiento
        image[i] = image[i - 1]; //mueve todos los valores un lugar a la derecha desde 95 hasta 0 (se pierde el 91) 
        image[i + OLED_WIDTH] = image[i - 1 + OLED_WIDTH];  //mueve todos los valores un lugar a la derecha desde 191 hasta el 96
        // moviendo los 2 arreglos libero el primer byte ubicado en el arreglo de 0~95 (arreglo de arriba)
        // y el primer byte del segundo arreglo 96~191 (arreglo de abajo)
    }

    image[OLED_WIDTH] = 0;
    image[0] = 0;

    // Añado el nuevo valor dependeindo su valor al sector correspondiente
    if (value < 8) {
        // debe ingresar apartir de el Byte 4
        image[OLED_WIDTH] = (1 << (7 - value));
    } else {
        // debe ingresar apartir de el Byte 0
        image[0] = (1 << (15 - value));
    }
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

void vUART_ISR(void)
{
 //falta implementar
}

