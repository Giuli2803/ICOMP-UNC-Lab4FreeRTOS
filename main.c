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
#define mainSTATS_DELAY ((TickType_t)1000 / portTICK_PERIOD_MS)
#define mainSENSOR_TASK_PRIORITY (tskIDLE_PRIORITY + 3)             // Prioridad de la tarea 'GENERATOR'.
#define mainFILTER_TASK_PRIORITY (tskIDLE_PRIORITY + 2)             // Prioridad de la tarea 'FILTER'.
#define mainDISPLAY_TASK_PRIORITY (tskIDLE_PRIORITY + 2)            // Prioridad de la tarea 'DISPLAY'.
#define mainSTATS_TASK_PRIORITY (tskIDLE_PRIORITY + 1)              // Prioridad de la tarea 'STATS'.

/* Configuracion UART - note que no utiliza FIFO por lo que no es muy eficiente. */
#define mainBAUD_RATE (19200)

volatile unsigned long ulHighFrequencyTimerTicks = 0;

/* Declaración de funciones */
static void prvSetupHardware( void );
static void vNumberGeneratorTask(void *pvParameters);   //Task 1
static void vFilterTask(void *pvParameters);            //Task 2
static void vDisplayTask(void *pvParameters);           //Task 3
static void vStatsTask(void *pvParameters);             //Task 4
void UARTSend(const char *pucBuffer);
void addValueToSignal(unsigned char image[OLED_WIDTH * 2], int value);
void intToStr(int num, char *str);                      // Función para convertir un entero a un string
void vUART_ISR(void);
void vSetupHighFrequencyTimer(void);
void Timer0IntHandler(void);

/* Defino colas de mensajes para el envio de datos */
QueueHandle_t xSensorQueue;
QueueHandle_t xDisplayQueue;
QueueHandle_t xFilterQueue;

volatile int N = 1;

/*-----------------------------------------------------------*/

int main( void )
{
    /* Configuro los clocks, UART y el Display. */
    prvSetupHardware();

    /* Instancio las colas de mensajes */
    //xSensorQueue = xQueueCreate(10, sizeof(int));
    xFilterQueue = xQueueCreate(10, sizeof(int));
    xDisplayQueue = xQueueCreate(10,sizeof(int));

	/* Defino las tareas solicitadas */
    xTaskCreate(vNumberGeneratorTask, "NumGen", configMINIMAL_STACK_SIZE, NULL, mainSENSOR_TASK_PRIORITY, NULL);
    xTaskCreate(vFilterTask, "Filter", configMINIMAL_STACK_SIZE, NULL, mainFILTER_TASK_PRIORITY, NULL);
    xTaskCreate(vDisplayTask, "Display", configMINIMAL_STACK_SIZE, NULL, mainDISPLAY_TASK_PRIORITY, NULL);
    xTaskCreate(vStatsTask, "Stats", configMINIMAL_STACK_SIZE, NULL, mainSTATS_TASK_PRIORITY, NULL);

	/* inicio el scheduler. */
	vTaskStartScheduler();

	return 0;
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
	/* Setup the PLL. */
	SysCtlClockSet( SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_6MHZ );

    vSetupHighFrequencyTimer();

	/* Initialise the LCD */
    OSRAMInit( false );
    OSRAMStringDraw("www.FreeRTOS.org", 0, 0);
	OSRAMStringDraw("LM3S811 demo", 16, 1);


    
    /* Enable the UART. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    /* Configure the UART for 8-N-1 operation. */
    UARTConfigSet(UART0_BASE, mainBAUD_RATE, UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE);

    // Habilitar las interrupciones del UART0
    UARTIntRegister(UART0_BASE, vUART_ISR); // Registrar la ISR
    IntEnable(INT_UART0); // Habilitar la interrupción UART0 en el NVIC
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT); // Habilitar interrupciones de recepción y tiempo de espera
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
        
        //xQueueSend(xDisplayQueue, &number, portMAX_DELAY);
        xQueueSend(xFilterQueue, &number, portMAX_DELAY);

        /* Incrementa el numero que simula el sensor. */
        number = (number + 1) % 16;

    }
}

static void vFilterTask(void *pvParameters)
{
    int sensorValue = 0;
    static int filterBuffer[MAX_FILTER_SIZE] = {0};
    int sum = 0;

    for (;;)
    {
        xQueueReceive(xFilterQueue, &sensorValue, portMAX_DELAY);
      
        /* Shift values */
        for(int i = MAX_FILTER_SIZE - 1; i > 0; i--)
        {
            filterBuffer[i] = filterBuffer[i - 1];
        }

        /* Add new value */
        filterBuffer[0] = sensorValue;

        sum = 0;
        /* Calculate average with N values*/
        for(int i = 0; i < N; i++)
        {
            sum += filterBuffer[i];
        }

        sum = sum / N;
        
        xQueueSend(xDisplayQueue, &sum, portMAX_DELAY); 
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
    TickType_t xLastExecutionTime;
    xLastExecutionTime = xTaskGetTickCount(); // Inicializa la variable xLastExecutionTime con el valor actual de ticks.

    // Variables para almacenar la información de las tareas
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize;
    uxArraySize = uxTaskGetNumberOfTasks(); // retorna el número de tareas en el sistema
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t)); // Reserva memoria para almacenar la información de las tareas
    
    if (pxTaskStatusArray == NULL) {
        for (;;)
        ;
    }

    for (;;) {

        vTaskDelayUntil(&xLastExecutionTime, mainSTATS_DELAY);
        volatile UBaseType_t x;
        unsigned int ulTotalRunTime, ulStatsAsPercentage;
        char temp[10] = "";

        UARTSend("\x1B[2J\x1B[H"); // ANSI command to clear screen
        UARTSend("----- Stats of the system -----\r\n");
        UARTSend("Task\tCPU %\tStatus\tStack HighWaterMark\r\n");

        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
        /* For percentage calculations. */
        ulTotalRunTime /= 100UL;

        // Recorro structura recibida de la tarea
        for (x = 0; x < uxArraySize; x++) {

            UARTSend(pxTaskStatusArray[x].pcTaskName);
            UARTSend("\t");

            if (ulTotalRunTime >0) 
            {
                ulStatsAsPercentage = pxTaskStatusArray[x].ulRunTimeCounter / ulTotalRunTime;
                if (ulStatsAsPercentage == 0) 
                {
                    UARTSend("0");
                } else {
                    intToStr(ulStatsAsPercentage, temp);
                    UARTSend(temp);
                }
            } else 
            {
                UARTSend("-");
            }

            UARTSend("\t");

            switch (pxTaskStatusArray[x].eCurrentState) 
            {
                case eRunning:
                UARTSend("Running");
                break;
                case eReady:
                UARTSend("Ready");
                break;
                case eBlocked:
                UARTSend("Blocked");
                break;
                case eSuspended:
                UARTSend("Suspended");
                break;
                case eDeleted:
                UARTSend("Deleted");
                break;
                case eInvalid:
                UARTSend("Invalid");
                break;
            }

            UARTSend("\t");
            intToStr(pxTaskStatusArray[x].usStackHighWaterMark, temp);
            UARTSend(temp);
            UARTSend("\r\n");
        }
    }
}


/**
 * @brief Adds a value to the OLED signal array and shifts the existing values.
 * @param image The signal array.
 * @param value The value to add.
 */
void addValueToSignal(unsigned char image[OLED_WIDTH * 2], int value) 
{
    // shift signal
    for (int i = OLED_WIDTH - 1; i > 0; i--) { 
        image[i] = image[i - 1];                            //mueve todos los valores un lugar a la derecha desde 95 hasta 0 (se pierde anterior del 95) 
        image[i + OLED_WIDTH] = image[i - 1 + OLED_WIDTH];  //mueve todos los valores un lugar a la derecha desde 191 hasta el 96 (se pierde anterior del 191) 
        // moviendo los 2 arreglos libero el primer byte ubicado en el arreglo de 0~95 (arreglo de arriba) y el primer byte del segundo arreglo 96~191 (arreglo de abajo)
    }

    image[OLED_WIDTH] = 0;  // borro el valor del primer byte (arreglo de abajo)
    image[0] = 0;           // borro el valor del primer byte (arreglo de arriba)

    // Añado el nuevo valor pero dependiendo su valor es al sector que corresponde (arriba o abajo)
    if (value < 8) {
        // Abajo
        image[OLED_WIDTH] = (1 << (7 - value));
    } else {
        // Arriba
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
    uint32_t ui32Status;
    char c;

    /* Get the interrupt status. */
    ui32Status = UARTIntStatus(UART0_BASE, true);

    /* Clear the asserted interrupts. */
    UARTIntClear(UART0_BASE, ui32Status);

    /* Loop while there are characters in the receive FIFO. */
    while(UARTCharsAvail(UART0_BASE))
    {
        /* Read the next character from the UART and store it in the buffer. */
        c = UARTCharGet(UART0_BASE);

        /* Check for end of line (enter key). */
        if (c == '+') 
        {
            N++;
            if (N > MAX_FILTER_SIZE) 
            {
                N = MAX_FILTER_SIZE;
            }
        }
        else if (c == '-') 
        {
            N--;
            if (N < 1) 
            {
                N = 1;
            }
        }
    }
}

void UARTSend(const char *pucBuffer)
{
    while (*pucBuffer != '\0') {
        UARTCharPut(UART0_BASE, *pucBuffer);
        pucBuffer++;
    }
}

/*-----------------------------Timer 0------------------------------*/

void vSetupHighFrequencyTimer(void) 
{
    // Habilito el Timer0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE,TIMER_CFG_32_BIT_TIMER);

    IntPrioritySet(INT_TIMER0A, 0);

    IntMasterEnable();
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    
    /* Bajar el 700 aumenta la precision del calculo */
    TimerLoadSet(TIMER0_BASE, TIMER_A, 90);
    TimerIntRegister(TIMER0_BASE,TIMER_A,Timer0IntHandler);
    TimerEnable(TIMER0_BASE,TIMER_A);
}

void Timer0IntHandler(void) {
  TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

  /* Keep a count of the total number of 20KHz ticks.  This is used by the
  run time stats functionality to calculate how much CPU time is used by
  each task. */
  ulHighFrequencyTimerTicks++;
}

/*-----------------------------------------------------------*/
