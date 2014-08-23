
#include "cpu_asm.h"

.text
.align 4
.globl _start
_start:
    b   _start_1

_start_1:
    cps     #(MODE_SVC)
    ldr     sp, =g_stack_svc

    cps     #(MODE_SYS)
    ldr     sp, =g_stack_sys
    // fiq
    cps     #(MODE_FIQ)
    ldr     sp, =g_stack_fiq
    // irq
    cps     #(MODE_IRQ)
    ldr     sp, =g_stack_irq

    // abt
    cps     #(MODE_ABT)
    ldr     sp, =g_stack_abt

    // undef
    cps     #(MODE_UNDEF)
    ldr     sp, =g_stack_undef

    cps     #(MODE_SYS)

    b       main