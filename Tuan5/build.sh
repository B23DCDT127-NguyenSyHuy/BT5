#!/bin/bash
arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O0 -nostdlib \
-I. -IFreeRTOS/include -IFreeRTOS/portable/GCC/ARM_CM3 \
main.c startup.c \
FreeRTOS/list.c FreeRTOS/queue.c FreeRTOS/tasks.c FreeRTOS/timers.c \
FreeRTOS/portable/GCC/ARM_CM3/port.c \
FreeRTOS/portable/MemMang/heap_4.c \
-T stm32f103.ld -o main.elf

arm-none-eabi-objcopy -O binary main.elf main.bin
st-flash write main.bin 0x08000000
