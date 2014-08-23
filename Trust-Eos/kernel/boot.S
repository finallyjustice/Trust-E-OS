

    .text
    .global _start
_start:
    ldr     r1, =0x1D3
    msr     cpsr, r1
    ldr     r0, =0x0
    ldr     r1, =0x00000998
    adr     r2, atags
    ldr     r3, =0x20000000
    ldr     r4, =0xC3E9BFB0
    ldr     r5, =0x0
    ldr     r6, =0x20008000
    ldr     r7, =0x300591A8
    ldr     r8, =0xC3E9BFDC
    ldr     r9, =0x20000100
    ldr     r10, =0x00000998
    ldr     r11, =0xC3EA5508
    ldr     r12, =0xFFFFFFD0
    ldr     lr, =kernel
    mov     pc, lr

    /*
      R0  0x00000000          
    R1  0x00000998          
    R2  0x20000100          
    R3  0x20000000          
    R4  0xC3E9BFB0          
    R5  0x00000000          
    R6  0x20008000          
    R7  0x300591A8          
    R8  0xC3E9BFDC          
    R9  0x20000100          
    R10 0x00000998          
    R11 0xC3EA5508          
    R12 0xFFFFFFD0          
    R13 0xC3FFED6C          
    R14 0x30002A74          
    PC  0x30028C44          
    CPSR    0x600001D3          
    SPSR    0x00000000          
User_System             
    */

    .org    0x100
atags:
    .long    0x00000005 
    .long    0x54410001 
    .long    0x00000000 
    .long    0x00000000 
    .long    0x00000000 
    .long    0x00000004 
    .long    0x54410002 
    .long    0x20000000 
    .long    0x20000000 
    .long    0x00000004 
    .long    0x54410002 
    .long    0x20000000 
    .long    0x40000000 
    .long    0x00000015 
    .long    0x54410009 
    .long    0x746F6F72 
    .long    0x65642F3D 
    .long    0x746D2F76 
    .long    0x6F6C6264 
    .long    0x20346B63 
    .long    0x746F6F72 
    .long    0x79747366 
    .long    0x793D6570 
    .long    0x73666661 
    .long    0x6E692032 
    .long    0x2F3D7469 
    .long    0x74696E69 
    .long    0x6E6F6320 
    .long    0x656C6F73 
    .long    0x7974743D 
    .long    0x30434153 
    .long    0x3531312C 
    .long    0x00303032 
    .long    0xFFFFFFFF 
    .long    0x00000005 
    .long    0x41001099 
    .long    0x00080000 
    .long    0x00400000 
    .long    0x03000000 
    .long    0x00000000 
    .long    0x00000000 
    .long    0xFFFFFFFF 
    .long    0xFFFFFFFF 
    .long    0xFFFFFFFF 
    .long    0xFFFFFFFF 
    .long    0xFFFFFFFF 
    .long    0xFFFFFFFF

