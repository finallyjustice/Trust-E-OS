#include "cpu_mmu.h"

pa_t va_to_pa_ns(va_t va)
{
#ifdef S5PV210

    pa_t pa;
    asm volatile("mcr p15, 0, %0, c7, c8, 4\n"::"r"(va):"memory", "cc");


    asm volatile("isb \n\t" 
                 " mrc p15, 0, %0, c7, c4, 0\n\t" 
                 : "=r" (pa) : : "memory", "cc"); 

    return (pa & 0xfffff000) | (va & 0xfff);

#elif FVP_EB_Cortex_A8
    return va;
#else
#error NOT SUPPORTED BOARD
#endif
}


void* map_from_ns(va_t va)
{
#ifdef S5PV210
    return (void *)va_to_pa_ns(va);
#elif FVP_EB_Cortex_A8
    return va;
#else
#error NOT SUPPORTED BOARD
#endif
}