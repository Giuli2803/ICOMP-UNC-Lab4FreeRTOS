# ICOMP-UNC-Lab4FreeRTOS

## Consigna:

En base a la Demo mostrada en clase del procesador `CORTEX_LM3S102_GCC`. Se debe modificar la demo para la realización de las siguientes tareas:

- Task 1: Simula un sensor de temperatura determinado. Con una frecuencia de 10Hz (10 muestras por segundo).
- Task 2: Recibe los valores del sensor y aplica un filtro pasa bajo. (Realiza el promedio de los N valores definidos)
- Task 3: Grafica los valores de la temperatura en el tiempo.
- Task 4: Implementar una tareas que muestre periodicamente estadistica de las tareas. Ej: top (muestra el uso del cpu, uso de memoria, etc)

### Requerimientos extra

Ademas de estas tareas el programa debe tener las siguientes caracteristicas:

- Se debe calcular el stack necesario ya sea con el formato Water Mark o con el formato Hook.
- Para comunicar las tareas entre si se utiliza Queues.
- Se debe poder recibir por UART el numero N que desea que aplique el filtro.
- Para Graficar el sensor en funcion del tiempo se utiliza la función OSRAMImageDraw
- Mediante el void vUART_ISR(void) debo recibir un N valido para aplicar al filtro pasabajo.

## Instrucciones para ejecutar:
Se debe tener instalado el compilador (Para trabajar en compilación cruzada):
```sh 
arm-none-eab-gcc
```
El comando para construir la solución, se genera el .axf en la carpeta gcc:
```sh
make
``` 
Para ejecutar la emulación con quemu desde la carpeta principal se puede usar el comando:

```sh 
sudo qemu-system-arm -machine lm3s811evb -kernel ./gcc/RTOSDemo.axf
```
El siguiente comando agrega ```-serial stdio``` al comando anterior que sirve para redirigir la salida del UART a la terminal y enviarle entradas:

```sh
sudo qemu-system-arm -machine lm3s811evb -kernel ./gcc/RTOSDemo.axf -serial stdio
```

## Simulación del sensor de temperatura
La simulación del sensor de temperatura se hizo mediante una tarea en el sistema. La cual genera Una señal tipo cierra, la cual va generando un numero ascendente hasta llegar al maximo numero de temperatura permitido ```MAX_TEMP_MEASUREMENTS```.

```sh
static void vNumberGeneratorTask(void *pvParameters)
{
    int number = 0;
    int normalizado = 0;
	TickType_t xLastExecutionTime;

	/* Inicializa la variable xLastExecutionTime con el valor actual de ticks.*/
	xLastExecutionTime = xTaskGetTickCount();

    for (;;)
    {	
		vTaskDelayUntil(&xLastExecutionTime, mainGENERATOR_DELAY); // Define el periodo de ejecución de la tarea

        /* Incrementa el numero que simula el sensor. */
        number = (number + 1) %MAX_TEMP_MEASUREMENTS;

        normalizado = (number * 16) / MAX_TEMP_MEASUREMENTS;

        /* Envio numero por la Cola de mensajes */
        xQueueSend(xFilterQueue, &normalizado, portMAX_DELAY);

    }
}
```
## Filtro de señales de temperatura
Se encaró el problema declarando una tarea la cual mediante una cola de mensajes recibe las muestras generadas por y las introduce en un arreglo el cual va Shifteando las muestras para no perder la información en el tiempo.
```sh
static void vFilterTask(void *pvParameters)
{
    int sensorValue = 0;
    static int filterBuffer[MAX_FILTER_SIZE] = {0};
    int sum = 0;

    for (;;)
    {
        xQueueReceive(xFilterQueue, &sensorValue, portMAX_DELAY);
      
        /* Shiftea los valores. Elimina el ultimo y deja libre el primero para ser sobre escrito */
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
```
Para el control de la variable `N` se utiliza comunicación UART para el ingreso de caracteres por la consola y asi lograr un incremento de la variable `N` y por lo tanto un incremento en la atenuación que realiza el filtro. Esto se Logro mediante los handlers de UART.
```sh
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
```
## Funcionamiento pantalla 96x16 

En la Funcion "OSRAMImageDraw" definida en el osram96x16.c se puede observar los parametros y la documentacion de esta funcion la cual permite mostrar en el display de 96px16p una imagen enviada como parametro.

La imagen se encuentra definida de la forma `const unsigned char *pucImage` en la cual podemos observar que cada valor del arreglo que debe recibir es un unsigned char, el cual tiene un valor de 1 byte u 8 bits.
Otros parametros importantes son `ulWidth` y `ulHeight`, en donde el primero es el largo de la imagen especificado en cantidad de columnas y el segundo es el alto de la imagen especificado en cuantos bloques de filas (en este caso solo es valido enviar el valor 1 ó 2).

Mediante el grafico se observa que los primeros 96 valores (de 0-95) del arreglo corresponden a la primera mitad de la pantalla de arriba, mientras que la segunda mitad del arreglo (de 96-181) corresponden con los valores de la parte inferior de la pantalla (segunda fila).

Inicialización de vector imagen:

```C
static unsigned char imagen[OLED_WIDTH * 2] = {0}; //tamaño de 181 valores char
```

Justificación del tamaño:

```C
[OLED_WIDTH * 2] = [(char0),(char1),(char2), ...... ,(char95), (char96),(char97),(char98), ...... ,(char191)]
```
## Stats 

En las stats se busco mmediante UART ir mostrando cada 1 segundo los cambios producidos en cada tarea con el siguiente formato:
```C
TaskName CPU_use% State StackHighWaterMark
```
Formando cuatro columnas con la información requerida y filas por la cantidad de tareas que se tiene.

### Envio de caracteres por UART
En primer instancia para el envio de caracteres se utiliza la siguiente funcion:
```sh
void UARTSend(const char *pucBuffer)
{
    while (*pucBuffer != '\0') {
        UARTCharPut(UART0_BASE, *pucBuffer);
        pucBuffer++;
    }
}
```
Esta funcion recibe solamente cadena de caracteres, por lo que tambien hizo falta una funcion auxiliar que convierte de entero a tipo char llamada `intToStr`.

### Desbordamiento de la pila

En este apartado se opto por utilizar la tecnica de detección WaterMark. La  misma se obtiene a travez de la funcion `uxTaskGetSystemState`, que cuando es llamada rellena la structura `TaskStatus_t` para cada tarea del sistema y en esta estructura se encuentra la variable `usStackHighWaterMark` esta indica la cantidad mínima de espacio de pila que queda para la tarea desde que se creó la tarea. Cuanto más cerca esté este valor de cero, más cerca estará la tarea de desbordar su pila.

### timer 0

Se utilizo para el conteo de los ticks un timer, en este caso el timer 0. Se utilizo como ejemplo una demo que tenia un ejemplo de como se utilizaba el mismo para el conteo.

Demo: CORTEX_LM3Sxxxx_Eclipse, del FreeRTOSv8.2.3

## Referencias

### Manejo de colas de mensajes:

https://www.freertos.org/a00018.html

### Stats
https://www.freertos.org/rtos-run-time-stats.html
https://www.freertos.org/uxTaskGetSystemState.html
https://sourceforge.net/projects/freertos/files/FreeRTOS/V8.2.3/

https://www.freertos.org/rtos-run-time-stats.html

### FreeRTOSConfig.h

https://www.freertos.org/a00110.html




