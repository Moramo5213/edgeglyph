# Toolchain paths
TOOLCHAIN_PATH = /Users/tensorcraft/.espressif/tools/riscv32-esp-elf/esp-15.2.0_20250929/riscv32-esp-elf/bin
CC = $(TOOLCHAIN_PATH)/riscv32-esp-elf-gcc
OBJCOPY = $(TOOLCHAIN_PATH)/riscv32-esp-elf-objcopy

# Compilation flags
CFLAGS = -march=rv32imc -mabi=ilp32 -static -mcmodel=medany \
         -fvisibility=hidden -nostdlib -nostartfiles -ffreestanding \
         -Wall -O2 -g

# Linker flags
LIBGCC = $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)
LDFLAGS = -T linker.ld -nostdlib $(LIBGCC)

# Files
SRCS = src/boot.S src/main.c
OBJS = src/boot.o src/main.o
TARGET = inference.elf
BINARY = inference.bin

PORT = /dev/cu.usbmodem11401

all: $(BINARY)

$(BINARY): $(TARGET)
	$(OBJCOPY) -O binary $(TARGET) $(BINARY)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)

src/%.o: src/%.S
	$(CC) $(CFLAGS) -c $< -o $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

flash: $(BINARY)
	esptool --chip esp32c3 --port $(PORT) --baud 921600 write_flash 0x0 $(BINARY)

clean:
	rm -f $(OBJS) $(TARGET) $(BINARY)

.PHONY: all clean flash
