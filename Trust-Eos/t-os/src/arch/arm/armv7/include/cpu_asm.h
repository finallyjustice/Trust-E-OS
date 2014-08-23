/*
 * 用于定义一些在*.S以及*.c文件中需要使用的宏
 */

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


// Cortex-A8 配置

/* 定义Nonsecure Access Control Register相关的值
 * 该寄存器用来定义不安全执行环境对协处理器的访问权限 P134 (Cortex-A8)
 */
#define NSACR_VALUE      (0x73FFF)

/* 寄存器Secure Configuration Register相关值 
 * 该寄存器功能：
 * 1.可以查看当前核执行的环境（安全或者非安全）
 * 2.配置异常所执行的环境
 * 3.配置在非安全环境使其是否可以修改CPSR的A、I位。
 */
#define SCR_NS_BIT      (1<<0)
#define SCR_IRQ_BIT     (1<<1)
#define SCR_FIQ_BIT     (1<<2)
#define SCR_EA_BIT      (1<<3)
#define SCR_FW_BIT      (1<<4)
#define SCR_AW_BIT      (1<<5)

// 软中断 - 任务调度标识
#define SVC_SCHEDULE_FLAG  (0xF000)

#endif