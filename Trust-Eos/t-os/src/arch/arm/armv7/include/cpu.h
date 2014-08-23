#ifndef _ARCH_ARMV7_CPU_H_
#define _ARCH_ARMV7_CPU_H_

// CPU通用寄存器
typedef struct _CPU_CORE_REGS {
    // R0,R2,...,R12 
    uint32_t r0, r1, r2, r3, r4, r5, r6;
    uint32_t r7, r8, r9, r10, r11, r12;
    // R13 - SP
    uint32_t sp;
    // R14 - LR
    uint32_t lr;
    // R15 - PC
    uint32_t pc;
    // SPSR
    uint32_t spsr;
}CPU_CORE_REGS;

#define read_dfsr()     ({ uint32_t rval; asm volatile(\
                " mrc     p15, 0, %0, c5, c0, 0\n\t" \
                : "=r" (rval) : : "memory", "cc"); rval;})

/**
 * @brief 
 */
#define read_dfar()     ({ uint32_t rval; asm volatile(\
                " mrc     p15, 0, %0, c6, c0, 0\n\t" \
                : "=r" (rval) : : "memory", "cc"); rval;})


#endif // tee_cpu.h