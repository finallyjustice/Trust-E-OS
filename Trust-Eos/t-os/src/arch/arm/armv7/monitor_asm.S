#include "monitor_asm.h"
#include "cpu_asm.h"

// 该宏的功能是将当前cpu寄存器中的值复制到结构体OS_CONTEXT变量context中。
.macro SAVE_CONTEXT context
    // current is monitor mode
    stmfd  sp!, {lr}                    // push lr_monitor
    
    // context.r0 ~ r12 = r0 ~ r12;
    ldr     lr, =\context
    stmia   lr!, {r0-r12}               // now lr_mon = &context.sp_usr

    // 保存当前上下文r13,r14,r15,cpsr
    cps     #MODE_SYS                   // change to system mode
    mov     r0, sp                      // r0 = sp_usr
    mov     r1, lr                      // r1 = lr_usr

    cps     #MODE_MON                   // change to monitor mode
    mrs     r2, spsr                    // r2 = spsr_mon = cpsr
    stmia   lr!, {r0-r2}                // context.sp_usr = r0 = sp_usr
                                        // context.lr_usr = r1 = lr_usr
                                        // context.cpsr = r2 = spsr_mon
                                        // now lr_mon = &context.r8_fiq

    cps     #MODE_FIQ
    mov     r0, r8                      // r0 =r8_fiq
    mov     r1, r9                      // r1 = r9_fiq
    mov     r2, r10                     // r2 = r10_fiq
    mov     r3, r11                     // r3 = r11_fiq
    mov     r4, r12                     // r4 = r12_fiq
    mov     r5, sp                      // r5 = sp_fiq
    mov     r6, lr                      // r6 = lr_fiq
    mrs     r7, spsr                    // r7 = spsr_fiq
    cps     #MODE_MON
    stmia   lr!, {r0-r7}                // context.r8_fiq ~ spsr_fiq = r0 ~ r7 = r8 ~ spsr_fiq
                                        // now lr_mon = &context.sp_irq

    cps     #MODE_IRQ
    mov     r0, sp                      // r0 = sp_irq
    mov     r1, lr                      // r1 = lr_irq
    mrs     r2, spsr                    // r2 = spsr_irq
    cps     #MODE_MON
    stmia   lr!, {r0-r2}                // context.sp_irq~spsr_irq = r0~r2 = sp_irq ~ spsr_irq
                                        // now lr_mon = &context.sp_svc
    cps     #MODE_SVC
    mov     r0, sp                      // r0 = sp_svc
    mov     r1, lr                      // r1 = lr_svc
    mrs     r2, spsr                    // r2 = spsr_svc
    cps     #MODE_MON
    stmia   lr!, {r0-r2}                // context.sp_svc~spsr_svc = r0~r2 = sp_svc ~ spsr_svc
                                        // now lr_mon = &context.sp_abt
    cps     #MODE_ABT
    mov     r0, sp                      // r0 = sp_abt
    mov     r1, lr                      // r1 = lr_abt
    mrs     r2, spsr                    // r2 = spsr_abt
    cps     #MODE_MON
    stmia   lr!, {r0-r2}                // context.sp_abt~spsr_abt = r0~r2 = sp_abt ~ spsr_abt
                                        // now lr_mon = &context.sp_undef

    cps     #MODE_UNDEF
    mov     r0, sp                      // r0 = sp_undef
    mov     r1, lr                      // r1 = lr_undef
    mrs     r2, spsr                    // r2 = spsr_undef
    cps     #MODE_MON
    stmia   lr!, {r0-r2}                // context.sp_undef~spsr_undef = r0~r2 = sp_undef ~ spsr_undef
                                        // now lr_mon = &context.pc
    mov     r0, lr                      // r0 = lr_mon = &context.pc
    ldmfd   sp!, {lr}                   // pop {lr}
    stmia   r0!, {lr}                   // context.pc = lr_mon
.endm

// 该宏将结构体OS_CONTEXT变量context所对应寄存器值恢复到CPU寄存器中
// 执行该宏语句的时候，当前模式必须为Monitor
.macro  RESTORE_CONTEXT context
    // current is monitor mode
    ldr     lr, =\context               // lr_mon = &context.r0
    ldmia   lr!, {r0-r12}               // r0~r12 = context.r0~r12
                                        // now lr_mon = &context.sp_usr

    //stmfd   sp!, {r0-r8}              // push {r0-r8}
    push    {r0-r7}
    ldmia   lr!, {r0-r2}                // r0 = context.sp_usr
                                        // r1 = context.lr_usr
                                        // r2 = context.cpsr
                                        // now lr_mon = &context.r8_fiq
    cps     #MODE_SYS                   // system mode
    mov     sp, r0                      // sp_usr = r0 = context.sp_usr
    mov     lr, r1                      // lr_usr = r1 = context.lr_usr
    cps     #MODE_MON                   // monitor mode
    msr     spsr, r2                    // spsr_mon = r2 = context.cpsr
    // FIQ 
    ldmia   lr!, {r0-r7}
    cps     #MODE_FIQ
    mov     r8, r0                      // r8_fiq = r0 = context.r8_fiq
    mov     r9, r1                      // r9_fiq = r1 = context.r9_fiq
    mov     r10, r2                     // r10_fiq = r2 = context.r10_fiq
    mov     r11, r3                     // r11_fiq = r3 = context.r11_fiq
    mov     r12, r4                     // f12_fiq = r4 = context.r12_fiq
    mov     sp, r5                      // sp_fiq = r5 = context.sp_fiq
    mov     lr, r6                      // lr_fiq = r6 = context.lr_fiq
    msr     spsr, r7                    // spsr_fiq = r7 = context.spsr_fiq
    cps     #MODE_MON                   // now lr_mon = &context.sp_irq
    // IRQ
    ldmia   lr!, {r0-r2}
    cps     #MODE_IRQ
    mov     sp, r0                      // sp_irq = r0 = context.sp_irq
    mov     lr, r1                      // lr_irq = r1 = context.lr_irq
    msr     spsr, r2                    // spsr_irq = r2 = context.spsr_irq
    cps     #MODE_MON                   // now lr_mon = &context.sp_svc
    // Supervisor
    ldmia   lr!, {r0-r2}
    cps     #MODE_SVC
    mov     sp, r0                      // sp_svc = r0 = context.sp_svc
    mov     lr, r1                      // lr_svc = r1 = context.lr_svc
    msr     spsr, r2                    // spsr_svc = r2 = context.spsr_svc
    cps     #MODE_MON                   // now lr_mon = &context.sp_abt
    // Abort
    ldmia   lr!, {r0-r2}
    cps     #MODE_ABT
    mov     sp, r0                      // sp_abt = r0 = context.sp_abt
    mov     lr, r1                      // lr_abt = r1 = context.lr_abt
    msr     spsr, r2                    // spsr_abt = r2 = context.spsr_abt
    cps     #MODE_MON                   // now lr_mon = &context.sp_undef
    // Undefine
    ldmia   lr!, {r0-r2}
    cps     #MODE_UNDEF
    mov     sp, r0                      // sp_undef = r0 = context.sp_undef
    mov     lr, r1                      // lr_undef = r1 = context.lr_undef
    msr     spsr, r2                    // spsr_undef = r2 = context.spsr_undef
    cps     #MODE_MON                   // now lr_mon = &context.pc
    ldmia   lr!, {r0}
    mov     lr, r0                      // lr_mon = context.pc
    pop     {r0-r7}
.endm

/* 
 * 监视器执行环境异常向量表（定义）
 */
.align 12
.global monitor_exception_vectors
monitor_exception_vectors:
    b   _monitor_reset_handler
    b   _monitor_undefined_handler
    b   _monitor_smc_handler
    b   _monitor_prefetch_handler
    b   _monitor_abort_handler
    /* 异常向量表保留位置 */
    b   .   
    b   _monitor_irq_handler
    b   _monitor_fiq_handler

_monitor_reset_handler:
    b   .
_monitor_undefined_handler:
    b   .

_monitor_smc_handler:
    cpsid   if
    push    {r1-r2}
    mrc     p15, 0, r1, c1, c1, 0
    ldr     r2, =SCR_NS_BIT
    bic     r1, r1, r2
    mcr     p15, 0, r1, c1, c1, 0
    pop     {r1-r2}
    cmp     r0, #SMC_ARG_LAUNCH_NON_SECURE
    beq     _launch_non_secure
    cmp     r0, #SMC_ARG_TZ_API
    beq     _call_tz_api
    cmp     r0, #SMC_ARG_TZ_API_RETURN
    beq     _launch_non_secure
    b       .


_monitor_prefetch_handler:
    b   .
_monitor_abort_handler:
    // 切换环境到安全执行环境来执行接下来的语句（避免异常)
    cpsid   if
    sub     lr, lr, #4
    push    {r1-r2}
    mrc     p15, 0, r1, c1, c1, 0
    ldr     r2, =SCR_NS_BIT
    bic     r1, r1, r2
    mcr     p15, 0, r1, c1, c1, 0
    pop     {r1-r2}
    SAVE_CONTEXT g_nonsecure_context
    ldr     r0, =g_nonsecure_context
    bl      monitor_data_abort_c_handler
    b       _restore_nonsecure_context
    b   .
_monitor_irq_handler:
    b   .
_monitor_fiq_handler:
    b   .

// nonsecure context call trustzone api
_call_tz_api:
    // current is nonsecure and monitor mode

_save_nonsecure_context:

    // 切换环境到安全执行环境来执行接下来的语句（避免异常）
    push    {r1-r2}
    mrc     p15, 0, r1, c1, c1, 0
    ldr     r2, =SCR_NS_BIT
    bic     r1, r1, r2
    mcr     p15, 0, r1, c1, c1, 0
    pop     {r1-r2}
    SAVE_CONTEXT g_nonsecure_context
    
_restore_secure_context:
    RESTORE_CONTEXT g_secure_context
    // 切换CPU运行环境到secure并且返回t-os执行
    /*
    mrc     p15, 0, r1, c1, c1, 0
    ldr     r2, =SCR_NS_BIT
    bic     r1, r1, r2
    mcr     p15, 0, r1, c1, c1, 0
    */
    movs    pc, lr

// launch non secure
_launch_non_secure:
    // current monitor mode
_save_secure_context:
    SAVE_CONTEXT g_secure_context
    // 至此当前运行环境下的CPU寄存器值已经复制到
    // 结构体OS_CONTEXT变量g_secure_context中

_restore_nonsecure_context:
    // current is monitor mode
    RESTORE_CONTEXT g_nonsecure_context
    // 至此寄存器已经恢复到nonsecure进入monitor时候的cpu状态

    // 切换CPU运行环境到nonsecure并且返回client执行
    mrc     p15, 0, r1, c1, c1, 0
    ldr     r2, =SCR_NS_BIT
    orr     r1, r1, r2
    ldr     r2, =SCR_EA_BIT
    orr     r1, r1, r2
    mcr     p15, 0, r1, c1, c1, 0
    clrex
    isb
    dsb
    movs    pc, lr
///////////////// RESTORE_NONSECURE_CONTEXT END   ///////////////////













/*************************abstract*************************************
_save_secure_context:
    stmfd  sp!, {lr}       // push lr_monitor
    
    // g_secure_context.r0 ~ r12 = r0 ~ r12;
    ldr     lr, =g_secure_context
    stmia   lr!, {r0-r12}   //now lr_mon = &context.sp_usr

    // 保存当前上下文r13,r14,r15,cpsr
    cps     #MODE_SYS       // change to system mode
    mov     r0, sp          // r0 = sp_usr
    mov     r1, lr          // r1 = lr_usr

    cps     #MODE_MON       // change to monitor mode
    mrs     r2, spsr        // r2 = spsr_mon = cpsr
    stmia   lr!, {r0-r2}    // context.sp_usr = r0 = sp_usr
                            // context.lr_usr = r1 = lr_usr
                            // context.cpsr = r2 = spsr_mon
                            // now lr_mon = &context.r8_fiq

    cps     #MODE_FIQ
    mov     r0, r8          // r0 =r8_fiq
    mov     r1, r9          // r1 = r9_fiq
    mov     r2, r10         // r2 = r10_fiq
    mov     r3, r11         // r3 = r11_fiq
    mov     r4, r12         // r4 = r12_fiq
    mov     r5, sp          // r5 = sp_fiq
    mov     r6, lr          // r6 = lr_fiq
    mrs     r7, spsr        // r7 = spsr_fiq
    cps     #MODE_MON
    stmia   lr!, {r0-r7}    // context.r8_fiq ~ spsr_fiq = r0 ~ r7 = r8 ~ spsr_fiq
                            // now lr_mon = &context.sp_irq

    cps     #MODE_IRQ
    mov     r0, sp          // r0 = sp_irq
    mov     r1, lr          // r1 = lr_irq
    mrs     r2, spsr        // r2 = spsr_irq
    cps     #MODE_MON
    stmia   lr!, {r0-r2}    // context.sp_irq~spsr_irq = r0~r2 = sp_irq ~ spsr_irq
                            // now lr_mon = &context.sp_svc
    cps     #MODE_SVC
    mov     r0, sp          // r0 = sp_svc
    mov     r1, lr          // r1 = lr_svc
    mrs     r2, spsr        // r2 = spsr_svc
    cps     #MODE_MON
    stmia   lr!, {r0-r2}    // context.sp_svc~spsr_svc = r0~r2 = sp_svc ~ spsr_svc
                            // now lr_mon = &context.sp_abt
    cps     #MODE_ABT
    mov     r0, sp          // r0 = sp_abt
    mov     r1, lr          // r1 = lr_abt
    mrs     r2, spsr        // r2 = spsr_abt
    cps     #MODE_MON
    stmia   lr!, {r0-r2}    // context.sp_abt~spsr_abt = r0~r2 = sp_abt ~ spsr_abt
                            // now lr_mon = &context.sp_undef

    cps     #MODE_UNDEF
    mov     r0, sp          // r0 = sp_undef
    mov     r1, lr          // r1 = lr_undef
    mrs     r2, spsr        // r2 = spsr_undef
    cps     #MODE_MON
    stmia   lr!, {r0-r2}    // context.sp_undef~spsr_undef = r0~r2 = sp_undef ~ spsr_undef
                            // now lr_mon = &context.pc
    mov     r0, lr          // r0 = lr_mon = &context.pc
    ldmfd   sp!, {lr}       // pop {lr}
    stmia   r0!, {lr}       // context.pc = lr_mon


*******************************************************************************/

