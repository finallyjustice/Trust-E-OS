OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH("arm")
ENTRY(_start)

PHY_OFFSET = 0x70008000;

SECTIONS
{
    . = PHY_OFFSET;
    
    _TZ_CODE_START = .;

    _TZ_TEXT_START = .;
    .text :
    {
        *start.o(.text)
        *(.text)
        *(.rodata .rodata.*)
    }
    _TZ_TEXT_END = .;

    _TZ_DATA_START = .; 
    .data :
    {
        *(.data)
    }
    . = ALIGN(4);
    _TZ_DATA_END = .;

    _TZ_BSS_START = .; 
    .bss :
    {
        *(.bss)
    }
    . = ALIGN(4);
    _TZ_BSS_END = .;

    . = ALIGN(0x10000);
    _CLIENT_START = .;
    .client :
    {
        *(.client)
    }
    _CLIENT_END = .;

    _TZ_CODE_END = .;
}