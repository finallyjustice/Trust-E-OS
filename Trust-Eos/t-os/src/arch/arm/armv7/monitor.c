#include "monitor.h"
#include "cpu_asm.h"
#include "tee_internal_api.h"

OS_CONTEXT g_secure_context;
OS_CONTEXT g_nonsecure_context;

void ExecuteSMC(uint32_t arg)
{
    asm volatile ("push {r0}");      // push {r0}
    register uint32_t r0 asm("r0") = arg;
    r0 = r0;
    asm volatile ("smc #0");
    asm volatile ("pop {r0}");       // push {r0}
}

void LaunchNonSecure()
{
    g_nonsecure_context.r0 = 0x0;//0x00000000;
    g_nonsecure_context.r3 = 0x0;// 0x03030303;
    g_nonsecure_context.r4 = 0x0;// 0x4;
    g_nonsecure_context.r5 = 0x0;// 0x5;
    g_nonsecure_context.r6 = 0x0;// 0x6;
    g_nonsecure_context.r7 = 0x0;// 0x7;
    g_nonsecure_context.r8 = 0x0;// 0x8;
    g_nonsecure_context.r9 = 0x0;// 0x9;
    g_nonsecure_context.r10 = 0x0;// 0x10;
    g_nonsecure_context.r11 = 0x0;// 0x11;
    g_nonsecure_context.r12 = 0x0;// 0x12;
    g_nonsecure_context.sp_usr = 0x0;// 0x13;
    g_nonsecure_context.lr_usr = 0x0;// 0x14;
    g_nonsecure_context.cpsr = MODE_SYS | I_BIT | F_BIT;
    g_nonsecure_context.r8_fiq = 0x0;// 0x8;
    g_nonsecure_context.r9_fiq = 0x0;// 0x9;
    g_nonsecure_context.r10_fiq = 0x0;// 0x10;
    g_nonsecure_context.r11_fiq = 0x0;// 0x11;
    g_nonsecure_context.r12_fiq = 0x0;// 0x12;
    g_nonsecure_context.sp_fiq = 0x0;// 0x13;
    g_nonsecure_context.lr_fiq = 0x0;// 0x14;
    g_nonsecure_context.spsr_fiq = 0x0;// 0x0;
    g_nonsecure_context.sp_irq = 0x0;// 0x13;
    g_nonsecure_context.lr_irq = 0x0;// 0x14;
    g_nonsecure_context.spsr_irq = 0x0;// 0x0;
    g_nonsecure_context.sp_svc = 0x0;// 0x13;
    g_nonsecure_context.lr_svc = 0x0;// 0x14;
    g_nonsecure_context.spsr_svc = 0x0;// 0x0;
    g_nonsecure_context.sp_abt = 0x0;// 0x13;
    g_nonsecure_context.lr_abt = 0x0;// 0x14;
    g_nonsecure_context.spsr_abt = 0x0;// 0x0;
    g_nonsecure_context.sp_undef = 0x0;// 0x13;
    g_nonsecure_context.lr_undef = 0x0;// 0x14;
    g_nonsecure_context.spsr_undef = 0x0;
#ifdef S5PV210
    g_nonsecure_context.pc = 0x20000000;
#elif FVP_EB_Cortex_A8
    g_nonsecure_context.pc = 0x70000000;
#else
#error NO BOARD SUPPORTED
#endif

    ExecuteSMC(SMC_ARG_LAUNCH_NON_SECURE);
}

void TZApiReturn(int32_t ret)
{
    asm volatile ("push {r0}");
    register uint32_t r0 asm("r0") = SMC_ARG_TZ_API_RETURN;
    //register uint32_t r1 asm("r1") = ret;
    g_nonsecure_context.r0 = ret;
    r0 = r0;
    //r1 = r1;
    asm volatile ("smc #0");
    asm volatile ("pop {r0}");

}


TZ_CMD_DATA *GetNonsecureCommand(void)
{
    return (TZ_CMD_DATA *)g_nonsecure_context.r1;
}