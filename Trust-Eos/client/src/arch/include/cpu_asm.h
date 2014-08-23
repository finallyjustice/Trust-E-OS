#ifndef ARM_ARMV7_CPU_ASM_H
#define ARM_ARMV7_CPU_ASM_H

/* CPSR模式位 */
#define  MODE_USR       (0x10)
#define  MODE_FIQ       (0x11)
#define  MODE_IRQ       (0x12)
#define  MODE_SVC       (0x13)
#define  MODE_MON       (0x16)
#define  MODE_ABT       (0x17)
#define  MODE_UNDEF     (0x1B)
#define  MODE_SYS       (0x1F)

/* CPSR中断位 */
#define  I_BIT          (0x80)    
#define  F_BIT          (0x40)

/* 模式栈大小 */
#define STACK_SIZE        (4096)
#define STACK_SIZE_SHIFT  (12)


#endif // cpu_asm.h