BOOTLOADER	= boot.S
KERNEL		= /home/sky/Desktop/source/trustzone-all/utv210-kernel/arch/arm/boot/zImage

IMAGE       = s5pv210_zImage
LD_SCRIPT	= model.lds

CROSS_COMPILE	= /usr/local/arm/arm-2009q3/bin/arm-none-linux-gnueabi-

AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

all: $(IMAGE)

clean:
	rm -f $(IMAGE) boot.o

$(IMAGE): image.elf
	$(OBJCOPY) -O binary $< $@
image.elf: boot.o $(LD_SCRIPT) $(KERNEL)
	$(LD) -o $@ --script=$(LD_SCRIPT)

boot.o: $(BOOTLOADER)
	$(AS) -o $@ $<
