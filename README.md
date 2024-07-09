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

