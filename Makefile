# toolchain & flags
CC      := gcc
LD      := ld
CFLAGS  := -m32 -nostdlib -fno-builtin -fno-stack-protector -O2 -Wall -I src
LDFLAGS := -m elf_i386

# nom de l'ISO final (sans extension .iso)
NAME    := CellularAutomatKerna

# sources & objets
SRCS    := src/kernel.c src/ca.c
OBJS    := kernel.o ca.o

.PHONY: all clean

all: $(NAME).iso

# compilation de kernel.c → kernel.o
kernel.o: src/kernel.c src/ca.h
	$(CC) $(CFLAGS) -c $< -o $@

# compilation de ca.c → ca.o
ca.o: src/ca.c src/ca.h
	$(CC) $(CFLAGS) -c $< -o $@

# linkage : on passe bien tous les objets à ld
kernel.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -T linker.ld -o $@ $(OBJS)

# création de l'ISO bootable
$(NAME).iso: kernel.elf grub.cfg
	@mkdir -p iso/boot/grub
	@cp kernel.elf      iso/boot/kernel.elf
	@cp grub.cfg        iso/boot/grub/
	@grub-mkrescue -o $@ iso

clean:
	@rm -f *.o *.elf
	@rm -rf iso $(NAME).iso
