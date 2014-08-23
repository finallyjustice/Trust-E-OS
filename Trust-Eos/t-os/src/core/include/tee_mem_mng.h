#ifndef _CORE_MEM_MNG_H_H
#define _CORE_MEM_MNG_H_H
#include "tee_internal_api.h"

#define TEE_PHY_TOTAL_SIZE      (1 << 26)
#define TEE_PHY_MEM_START       (0x7F000000)
#define TEE_PHY_MEM_END         (0x82FFFFFF) //TEE_PHY_MEM_START + TEE_PHY_TOTAL_SIZE -1)

#define TEE_VIR_MEM_START       TEE_PHY_MEM_START
#define TEE_VIR_MEM_END         TEE_PHY_MEM_END

// page size:4096
#define TEE_PAGE_SHIFT          (12)
#define TEE_PAGE_SIZE           (1 << TEE_PAGE_SHIFT)

// functions (函数声明)
void TEE_MemInit(void);

// 分配num个Page
void *TEE_AllocPages(uint32_t num);

// 释放num个Page
void TEE_FreePages(void *addr, uint32_t num);

 

#endif