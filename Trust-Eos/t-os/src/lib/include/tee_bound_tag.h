#ifndef _LIB_TEE_BOUND_TAG_H_
#define _LIB_TEE_BOUND_TAG_H_

#include "tee_mem_mng.h"
#include "tee_internal_api.h"

// 内存池数量
#define MEM_POOL_NUM            (0x1)

// 内存池所占页数
#define MEM_POOL_PAGES_NUM      (1024)

// 边界标识的所指内存区域的头
typedef struct _MEM_HEAD {
    struct _MEM_HEAD *prev,*next;
    bool tag;
    uint32_t valid_size;
}HEAD __attribute__ ((aligned (4)));

// 边界标识所指内存区域的尾
typedef struct _MEM_FOOT {
    struct _MEM_HEAD *head;
}FOOT __attribute__ ((aligned (4)));

// 内存池
typedef struct _MEM_POOL{
    // 内存池的地址
    void *addr;
    // 内存池的大小
    uint32_t size;
    // 内存池空闲链表
    HEAD *free_list;
    // 被占用的链表
    // HEAD *used_list;
}MEM_POOL;

#define EPSILON     (4+sizeof(HEAD)+sizeof(FOOT))

void BoundTag_Init(MEM_POOL *mem_pool, void *addr, uint32_t size);
void *BoundTag_Alloc(MEM_POOL *mem_pool, uint32_t n);
void BoundTag_Free(MEM_POOL *mem_pool, void *addr);
void BoundTag_Display(MEM_POOL *mem_pool);


// 说明：判断所给地址是否是合法分配
bool BoundTag_ValidAllocated(void *addr);

// 说明：获取所给地址对应内存的valid_size
//       该地址的合法性应该由程序保证,
//       所以这里不做检查
uint32_t BoundTag_ValidSize(void *addr);



// 说明：测试“边界标识法”动态内存管理
void BoundTag_Test();



// 通过头得到边界下部指针
static inline uint32_t FootAddr(HEAD *head)
{
    return (uint32_t)head + head->valid_size + sizeof(HEAD);
}

// 通过内存地址获取边界上部头指针
static inline uint32_t HeadAddr(void *addr)
{
    return (uint32_t)addr - sizeof(HEAD);
}


#endif