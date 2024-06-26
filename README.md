# ICOMP-UNC-Lab4FreeRTOS

## Instrucciones para ejecutar:

- arm-none-eab-gcc (Tener instalado)
- make  (para construir la solución, se genera el .axf en la carpeta gcc)
- sudo qemu-system-arm -machine lm3s811evb -kernel ./gcc/RTOSDemo.axf (Ejecuto el la emulación con quemu desde la carpeta principal (FreeRTOS/Demo/CORTEX_LM3S811_GCC))

