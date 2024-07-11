# ICOMP-UNC-Lab4FreeRTOS

## Consigna:

En base a la Demo mostrada en clase del procesador 'CORTEX_LM3S102_GCC'. Se debe modificar la demo para la realización de las siguientes tareas:

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

- ```arm-none-eab-gcc```(Tener instalado)
- ```make``` (para construir la solución, se genera el .axf en la carpeta gcc)
- ```sudo qemu-system-arm -machine lm3s811evb -kernel ./gcc/RTOSDemo.axf```
(Ejecuto el la emulación con quemu desde la carpeta principal)

## Funcionamiento pantalla 96x16 

En la Funcion "OSRAMImageDraw" definida en el osram96x16.c se puede observar los parametros y la documentacion de esta funcion la cual permite mostrar en el display de 96px16p una imagen enviada como parametro.

La imagen se encuentra definida de la forma ```const unsigned char *pucImage``` en la cual podemos observar que cada valor del arreglo que debe recibir es un unsigned char, el cual tiene un valor de 1 byte u 8 bits.
Otros parametros importantes son ```ulWidth``` y ```ulHeight```, en donde el primero es el largo de la imagen especificado en cantidad de columnas y el segundo es el alto de la imagen especificado en cuantos bloques de filas (en este caso solo es valido enviar el valor 1 ó 2).

Mediante el grafico se observa que los primeros 96 valores (de 0~95) del arreglo corresponden a la primera mitad de la pantalla de arriba, mientras que la segunda mitad del arreglo (de 96~181) corresponden con los valores de la parte inferior de la pantalla (segunda fila).

Inicialización de vector imagen:

```static unsigned char imagen[OLED_WIDTH * 2] = {0}; //tamaño de 181 valores char```

Justificación del tamaño:

```[OLED_WIDTH * 2] = [(char0),(char1),(char2), ...... ,(char95), (char96),(char97),(char98), ...... ,(char191)]```




