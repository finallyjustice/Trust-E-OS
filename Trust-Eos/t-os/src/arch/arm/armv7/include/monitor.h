#ifndef _ARCH_ARMV7_MONITOR_H_
#define _ARCH_ARMV7_MONITOR_H_

#include "monitor_asm.h"
#include "tee_internal_api.h"
#include "cpu.h"
#include "communication/types.h"

typedef struct _OS_CONTEXT {
    // Usr
    uint32_t    r0, r1, r2, r3, r4, r5, r6, r7;
    uint32_t    r8, r9, r10, r11, r12;
    uint32_t    sp_usr;
    uint32_t    lr_usr;
    uint32_t    cpsr;
    // FIQ
    uint32_t    r8_fiq;
    uint32_t    r9_fiq;
    uint32_t    r10_fiq;
    uint32_t    r11_fiq;
    uint32_t    r12_fiq;
    uint32_t    sp_fiq;
    uint32_t    lr_fiq;
    uint32_t    spsr_fiq;
    // IRQ
    uint32_t    sp_irq;
    uint32_t    lr_irq;
    uint32_t    spsr_irq;
    // Supervisor
    uint32_t    sp_svc;
    uint32_t    lr_svc;
    uint32_t    spsr_svc;
    // Abort
    uint32_t    sp_abt;
    uint32_t    lr_abt;
    uint32_t    spsr_abt;
    // Undefined
    uint32_t    sp_undef;
    uint32_t    lr_undef;
    uint32_t    spsr_undef;

    // usr
    uint32_t    pc;
}OS_CONTEXT;

void ExecuteSMC(uint32_t arg);

void LaunchNonSecure();

void TZApiReturn(int32_t ret);

// 说明：获取从nonsecure传递过来的TZ_CMD_DATA地址
TZ_CMD_DATA *GetNonsecureCommand(void);

#endif // monitor.h