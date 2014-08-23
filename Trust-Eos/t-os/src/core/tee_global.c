#include "tee_internal_api.h"
#include "tee_global.h"
#include "tee_mem_mng.h"
#include "tee_bound_tag.h"
#include "tee_debug.h"
#include "tee_scheduler.h"

// 全局内存池 (目前仅用索引0对应的内存池)
extern MEM_POOL g_mem_pool[];


// 说明：全局必要初始化
void global_init(void)
{
    void *addr = NULL;
    MEM_POOL *mem_pool = &g_mem_pool[0];

    // 串口初始化
    debug_init();

    // 内存信息初始化
    TEE_MemInit();

    // 为全局内存分配页空间
    addr = TEE_AllocPages(MEM_POOL_PAGES_NUM);

    // 初始化全局内存池
    BoundTag_Init(mem_pool, addr, MEM_POOL_PAGES_NUM << TEE_PAGE_SHIFT);

    TaskInit();
}

