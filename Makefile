# (C) Copyright 2025 Chris January

TOOLCHAIN ?= m68k-elf-
CC = $(TOOLCHAIN)gcc
OBJCOPY = $(TOOLCHAIN)objcopy
OBJDUMP = $(TOOLCHAIN)objdump

ifndef NEXTP8_BSP
$(error Set NEXTP8_BSP to the path to the nextp8 BSP)
endif

CPPFLAGS += -isystem$(NEXTP8_BSP)/include
LDFLAGS += -L$(NEXTP8_BSP)/lib
ROM_LDFLAGS += -T$(NEXTP8_BSP)/nextp8-rom.ld
RAM_LDFLAGS += -T$(NEXTP8_BSP)/nextp8-ram.ld
CFLAGS += -Wall $(CPPFLAGS) -g
ROM_CFLAGS += -DROM=1

all: build/loader.bin build/loader.mem build/loader.lst

build/%.bin: build/%
	$(OBJCOPY) -O binary $< $@

build/%.mem: build/%.bin
	od -v -w2 -Ax -tx2 --endian=big $< | cut -d' ' -f2 -s > $@

build/%.lst: build/%
	$(OBJDUMP) -d -S $< > $@

build/loader: build/loader.o
	mkdir -p build
	$(CC) $(CFLAGS) $(ROM_CFLAGS) $(LDFLAGS) $(ROM_LDFLAGS) $^ -o $@ $(LIBS) -Wl,-Map=$@.map

build/%.o: src/%.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build

.PHONY: clean
