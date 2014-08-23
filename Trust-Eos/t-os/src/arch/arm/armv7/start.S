/*
 * T-OS入口，此文件代码为T-OS起始执行的代码，当前执行环境须为安全执行环境。
 * 1.安装安全执行环境异常向量表
 * 2.安装监视器执行环境异常向量表
 * 3.cpu初始化：mmu、cache等的初始化
 * 4.c程序的bss段的初始化
 * 5.进入c代码主函数main
 */

 #include "cpu_asm.h" 

 .text
 .align 4
 .globl _start
 _start:
    b   _start_1


/*
 * 安全执行环境异常向量表（定义）
 */
.align 12
.global secure_exception_vectors
secure_exception_vectors:
    b       _reset_handler
    b       _undefined_handler
    // 原软中断 swi
    b       _svc_handler        
    b       _prefetch_handler
    b       _abort_handler
    /* 异常向量表保留位置 */
    b       .
    b       _irq_handler
    b       _fiq_handler

_reset_handler:
    b       _start

_undefined_handler:
    b       .

// Supervisor Call 
_svc_handler:
    cmp     r0, #SVC_SCHEDULE_FLAG
    bne     _svc_other
_svc_task_schedule:
    // "now in supervisor mode" 
    stmfd   sp!, {lr}       // 临时存放lr占用
    ldr     lr, =g_temp_regs// lr = &g_temp_regs
    stmia   lr!, {r0-r12}   // 赋值temp_regs.r0-r12
    
    cps     #MODE_SYS       // "change to system mode"
    mov     r0, sp          // "r0 = sp_sys"
    mov     r1, lr          // "r1 = lr_sys"
    cps     #MODE_SVC       // change to supervisor mode
    mov     r4, lr          // r4 = &g_temp_regs.sp
    ldmfd   sp!, {lr}        // restore lr_svc
    mov     r2, lr          // r2 = lr
    mrs     r3, spsr        // r3 = spsr_svc
    stmia   r4!, {r0-r3}    // g_temp_regs.sp = r0 = sp_sys
                            // g_temp_regs.lr = r1 = lr_sys
                            // g_temp_regs.pc = r2 = lr_svc
                            // g_temp_regs.spsr = r3 = spsr_svc
    // 任务切换
    bl      TaskSwitch

    // 从g_temp_regs中恢复上下文执行
    ldr     lr, =g_temp_regs    // lr = &g_temp_regs.r0
    ldmfd   lr!, {r0-r12}       // r0-r12 = g_temp_regs.r0-r12
    stmfd   sp!, {r0-r4}        // push {r0-r4} to sp_svc for temporary use
    mov     r4, lr              // r4 = lr = &g_temp_regs.sp
    cps     #MODE_SYS           // chnge to system mode
    ldmfd   r4!, {r0-r3}        // r0 = g_temp_regs.sp
                                // r1 = g_temp_regs.lr
                                // r2 = g_temp_regs.pc
                                // r3 = g_temp_regs.spsr
    mov     sp, r0              // sp_sys = r0 = g_temp_regs.sp
    mov     lr, r1              // lr_sys = r1 = g_temp_regs.lr
    cps     #MODE_SVC           // change to supervisor mode
    mov     lr, r2              // lr_svc = g_temp_regs.pc
    msr     spsr, r3            // spsr_svc = r3 = g_temp_regs.spsr
    ldmfd   sp!, {r0-r4}        // pop {r0-r4}

    // 恢复任务执行
    stmfd   sp!, {lr}
    ldmfd   sp!, {pc}^



_svc_other:
    b	.

_prefetch_handler:
    b	.
_abort_handler:
    b	.
_irq_handler:
    b	.
_fiq_handler:
    b	.

_start_1:
// S5PV210 Cortex-A8 ，单核处理器 
    

// 安装异常向量表 
    // 安装安全执行环境异常向量表 
    ldr     r1, =secure_exception_vectors
    mcr     p15, 0, r1, c12, c0, 0

    // 安装监视器执行环境异常向量表 
    ldr     r1, =monitor_exception_vectors
    mcr     p15, 0, r1, c12, c0, 1

// 初始化堆栈 

    // FIQ模式 
    msr     cpsr_c, #(MODE_FIQ | I_BIT | F_BIT) 
    ldr     r1, =fiq_stack
    add     sp, r1, #STACK_SIZE

    // Supervisor模式
    msr     cpsr_c, #(MODE_SVC | I_BIT | F_BIT)
    ldr     r1, =supervisor_stack
    add     sp, r1, #STACK_SIZE

    // Abort模式 
    msr     cpsr_c, #(MODE_ABT | I_BIT | F_BIT)
    ldr     r1, =abort_stack
    add     sp, r1, #STACK_SIZE

    // IRQ模式 
    msr     cpsr_c, #(MODE_IRQ | I_BIT | F_BIT)
    ldr     r1, =irq_stack
    add     sp, r1, #STACK_SIZE
    // undefined模式
    msr     cpsr_c, #(MODE_UNDEF | I_BIT | F_BIT)
    ldr     r1, =undefined_stack
    add     sp, r1, #STACK_SIZE

    // monitor模式 
    msr     cpsr_c, #(MODE_MON | I_BIT | F_BIT)
    ldr     r1, =monitor_stack
    add     sp, r1, #STACK_SIZE

    // system/user模式,（系统/用户公用所有寄存器） 
    msr     cpsr_c, #(MODE_SYS | I_BIT | F_BIT)
    ldr     r1, =user_stack
    add     sp, r1, #STACK_SIZE

    // cpu初始化
    bl      cpu_init
    // c程序bss段初始化
    bl      c_bss_init
    b       main




// 原型：void c_bss_init
// 说明：C程序的bss数据段初始化
.global c_bss_init
.func c_bss_init
c_bss_init:
    push    {r0-r2}
    ldr     r1, =_TZ_BSS_START
    ldr     r2, =_TZ_BSS_END
    mov     r0, #0
    cmp     r1, r2          // if (r1 > r2)
    bge     2f              //    goto 2         
1:
    str     r0, [r1], #4    // *(int*)r1++ = r0
    cmp     r1, r2          // if (r1 < r2)
    blt     1b              // goto 1
2:
    pop     {r0-r2}
    mov     pc, lr
.endfunc

// 原型：void cpu_init(void)
// 说明：CPU初始化（mmu,cache）
.global cpu_init
.func cpu_init
cpu_init:
    push    {r1, r2, lr}

    // 禁止MMU 
    mrc     p15, 0, r1, c1, c0, 0       // 读取控制寄存器Control Register)
    bic     r1, r1, #0x1                // CR[0]:0 - MMU disabled 
    mcr     p15, 0, r1, c1, c0, 0       // 更新控制寄存器(Control Register)

    // 禁止Caches
    mrc     p15, 0, r1, c1, c0, 0       // 读取控制寄存器Control Register)
    bic     r1, r1, #(0x1 << 2)         // CR[2]:0 - data caching disabled at all levels
    bic     r1, r1, #(0x1 << 12)        // CR[12]:0 - instruction caching disabled at all levels
    mcr     p15, 0, r1, c1, c0, 0       // 更新控制寄存器(Control Register)

    // 作废指令caches
    mov     R1,#0
    mcr     p15, 0, r1, c7, c5, 0

    // 作废数据caches
    bl      clean_invalidate_dcache

    // 作废分支预测阵列
    mov     r1, #0
    mcr     p15, 0, r1, c7, c5,  6

    // 作废TLBS
    //转换旁视缓冲(Translation Look-aside Buffers)
    mov     r1, #0x0
    mcr     p15, 0, r1, c8, c7, 0       // Invalidate Inst-TLB and Data-TLB (P154)

    /**************************************************************************
    * mrc     p15, 0, r1, c1, c0, 0        @ Read SCTLR
    * bic     r1, r1, #0x10000000
    * bic     r1, r1, #0x00002000
    * mcr     p15, 0, r1, c1, c0, 0
    ****************************************************************************/

    // 配置Nonsecure Access Control Register
    ldr     r1, =NSACR_VALUE
    mcr     p15, 0, r1, c1, c1, 2       // 更新Nonsecure Access Control Register


    // 设置发生快速中断（FIQ）时跳转到Monitor模式执行
    mrc     p15, 0, r1, c1, c1, 0        @ Read Secure Configuration Register data
    ldr     r2, =SCR_FIQ_BIT             
    orr     r1, r1, r2  
    mcr     p15, 0, r1, c1, c1, 0        @ Write Secure Configuration Register data
    pop     {r1, r2, lr} 
    mov     pc, lr
.endfunc