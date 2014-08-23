#ifndef _ARCH_ARMV7_CPU_MMU_H_
#define _ARCH_ARMV7_CPU_MMU_H_

#include "tee_internal_api.h"

typedef uint32_t pa_t;
typedef pa_t     va_t;

pa_t va_to_pa_ns(va_t va);

// 将nonsecure的虚拟地址va映射到secure环境中，并且将其地址返回
void* map_from_ns(va_t va);

#endif