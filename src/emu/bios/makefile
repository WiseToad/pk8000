.PHONY: all

all: target/bios.coff32.o \
     target/bios.coff64.o \
     target/bios.elf32.o \
     target/bios.elf64.o

target/%.o: src/%.asm
	fasm $< $@

target/bios.coff32.o: bin/bios.bin

target/bios.coff64.o: bin/bios.bin

target/bios.elf32.o:  bin/bios.bin

target/bios.elf64.o:  bin/bios.bin
