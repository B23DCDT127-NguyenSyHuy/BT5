CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
CFLAGS = -mcpu=cortex-m3 -mthumb -O0 -Wall -nostdlib
LDFLAGS = -T stm32f103.ld

all: main.bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

main.elf: main.o startup.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

main.bin: main.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.o *.elf *.bin

flash: main.bin
	st-flash write main.bin 0x8000000
