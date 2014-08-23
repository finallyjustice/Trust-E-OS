OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
TARGET(binary)

INPUT(/media/Storage1/weilan/share/kernel/arch/arm/boot/zImage)
INPUT(./boot.o)

PHYS_OFFSET = 0x20000000;

SECTIONS
{
    . = PHYS_OFFSET;
    .text : { boot.o }

    . = PHYS_OFFSET + 0x8000;
    kernel = .;
    .kernel : { /media/Storage1/weilan/share/kernel/arch/arm/boot/zImage }

    .data : { *(.data) }
    .bss : { *(.bss) }
}