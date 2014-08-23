#include "tee_bound_tag.h"
#include "tee_debug.h"

// 动态内存管理：边界标识法
MEM_POOL g_mem_pool[MEM_POOL_NUM];

// MEM_POOL* GetMemPool()
// {
//     return &g_mem_pool[0];
// }

void BoundTag_Init(MEM_POOL *mem_pool, void *addr, uint32_t size)
{
    HEAD *head;
    FOOT *foot;
    head = (HEAD*)addr;
    head->valid_size = size - sizeof(HEAD)- sizeof(FOOT);
    head->tag = FALSE;
    head->prev = head;
    head->next = head;
    foot = (FOOT*)FootAddr(head);
    foot->head = head;
    mem_pool->free_list = head;
    mem_pool->addr = addr;
    mem_pool->size = size;
}

void *BoundTag_Alloc(MEM_POOL *mem_pool, uint32_t n)
{
    HEAD *head = mem_pool->free_list;
    HEAD *p = head; 
    
    // 4字节对齐 word 
    // 4字节对齐
    n = (n & 0x3) == 0 ? n : ((n & (~0x3)) + 4);
    n += sizeof(HEAD)+sizeof(FOOT);
    
    // 寻找可供分配的空闲块
    while (p && p->valid_size < n && p->next != head) {
        p = p->next;
    }
    if (!p || p->valid_size < n) {
        return NULL;
    }

    // 如果申请大小接近空闲块大小，直接将其完全分配
    if (p->valid_size - n < EPSILON) {
        p->tag = TRUE;                  // 标识已分配
        // 更新空闲块链表
        if (p == head) {  
            mem_pool->free_list = NULL;
        } 
        else {
            p->prev->next = p->next;
            p->next->prev = p->prev;
            p->next = p->prev = NULL;
        }
        // 返回分配的可用内存起始地址
        return (void*)((uint32_t)p + sizeof(HEAD));
    }
    // 在空闲块的分配分配出n字节空间
    else {
        FOOT *new_foot = (FOOT *)FootAddr(p);   // 所分配块的FOOT
        p->valid_size -= n;                    // 原空闲块大小更新
        FOOT *p_foot = (FOOT*)FootAddr(p);      // 原空闲块foot更新
        p_foot->head = p;                       // 原空闲块foot更新
        // 所分配块的head更新
        HEAD *new_head = (HEAD*)((uint32_t)p_foot + sizeof(FOOT));
        new_head->tag = TRUE;
        new_head->valid_size = n - sizeof(HEAD) - sizeof(FOOT);
        new_foot->head = new_head;
        new_head->prev = new_head->next = new_head;
        // 返回分配的可用内存起始地址
        return (void*)((uint32_t)new_head + sizeof(HEAD));
    }
}

// 回收内存
void BoundTag_Free(MEM_POOL *mem_pool, void *addr)
{
    if (!addr || !mem_pool) {
        return;
    }
    bool left_free,right_free;
    FOOT *left_foot,*right_foot;
    HEAD *left_head,*right_head;
    HEAD *free_head,*head;
    FOOT *free_foot;
    head = mem_pool->free_list;
    // 所释放的空闲块head及foot
    free_head = (HEAD *)HeadAddr(addr);
    free_foot = (FOOT *)FootAddr(free_head);
    free_head->tag = FALSE;
    // 如果当前空闲链表为空，直接更新
    if (!mem_pool->free_list) { 
        mem_pool->free_list = free_head;
        return;
    }

    // 根据释放块的左邻、右邻块的情况考虑回收过程
    // 获取释放块的左邻、右邻块
    left_free = right_free = TRUE;
    left_foot = (FOOT*)((uint32_t)free_head - sizeof(FOOT));
    left_head = left_foot->head;
    right_head = (HEAD*)((uint32_t)free_foot + sizeof(FOOT));
    right_foot = (FOOT*)FootAddr(right_head);
    if ((uint32_t)left_foot < (uint32_t)mem_pool->addr 
        || left_foot->head->tag)  {
        left_free = FALSE;
    }
    if ((uint32_t)right_head >= (uint32_t)mem_pool->addr + mem_pool->size
        || right_head->tag) {
            right_free = FALSE;
    }
    // 左右皆占，直接加入链表
    if (!left_free && !right_free) {
        head->next->prev = free_head;
        free_head->next = head->next;
        head->next = free_head;
        free_head->prev = head; 
    }

    // 左空、右占
    else if (left_free && !right_free) {
        left_head->valid_size += free_head->valid_size+sizeof(HEAD)+sizeof(FOOT);
        free_foot->head = left_head;
    }
    
    // 左占、右空
    else if (!left_free && right_free) {
        free_head->valid_size += right_head->valid_size
                                    + sizeof(HEAD) + sizeof(FOOT);
        free_head->next = right_head->next;
        free_head->next->prev = free_head;
        free_head->prev = right_head->prev;
        free_head->prev->next = free_head;
        right_foot->head = free_head;
        free_head->tag = FALSE;
        // 如果当前链表头指向的是即将被消失的right_head,
        // 更改
        if (right_head == mem_pool->free_list) {
            mem_pool->free_list = free_head;
        }
    }
    // 左空、右空
    else{
        // 如果当前链表头指向的是即将被消失的right_head,
        // 更待当前的链表头
        if (right_head == mem_pool->free_list) {
            mem_pool->free_list = left_head;
        }
        // 将right_head从链表中删除
        right_head->prev->next = right_head->next;
        right_head->next->prev = right_head->prev;

        // left_head,free_head,right_head合并
        left_head->valid_size += right_head->valid_size + free_head->valid_size
                                + ((sizeof(HEAD) + sizeof(FOOT)) << 1);
        right_foot->head = left_head; 
    }
}

// 说明：判断所给地址是否是合法分配
bool BoundTag_ValidAllocated(void *addr)
{
    HEAD *head = NULL;
    MEM_POOL *mem_pool = &g_mem_pool[0];
    if (addr < mem_pool->addr ||
        (uint32_t)addr >= (uint32_t)mem_pool->addr + mem_pool->size) {
        return FALSE;
    }

    head = (HEAD *)HeadAddr(addr);
    if (head->prev != NULL && head->next != NULL &&
        head->tag == TRUE && head->valid_size > 0 &&
        head->valid_size < mem_pool->size - sizeof(HEAD) - sizeof(FOOT)) {
        return TRUE;
    }
    return FALSE;
}

// 说明：获取所给地址对应内存的valid_size
//       该地址的合法性应该由程序保证,
//       所以这里不做检查
uint32_t BoundTag_ValidSize(void *addr)
{
    HEAD *head = (HEAD *)HeadAddr(addr);
    return head->valid_size;
}

// 内存池空间展示
void BoundTag_Display(MEM_POOL *mem_pool)
{
    HEAD *head,*p;
    uint32_t tick;
    head = mem_pool->free_list;
    p = head;
    tick = 0;
    TEE_Printf("\n==========Memory display==========\n");
    while (p) {
        TEE_Printf("block %d addr:0x%x  total size:%d   valid:size:%d\n",
                    tick++, p, p->valid_size + sizeof(HEAD) + sizeof(FOOT),
                    p->valid_size);
        if (p->next == head) {
            break;
        }
        p = p->next;
    }
    TEE_Printf("====================================\n");
}

void BoundTag_Test()
{
    int i;
    char *addr[5];
    MEM_POOL *mem_pool = &g_mem_pool[0];
    // 分配5次
    for (i=0; i<5; i++) {
        addr[i] = BoundTag_Alloc(mem_pool, 100);
        BoundTag_Display(mem_pool);
    }

    // 释放5次 释放次序为  2 0 1 4 3
    BoundTag_Free(mem_pool, addr[2]);
    BoundTag_Display(mem_pool);
    BoundTag_Free(mem_pool, addr[0]);
    BoundTag_Display(mem_pool);
    BoundTag_Free(mem_pool, addr[1]);
    BoundTag_Display(mem_pool);
    BoundTag_Free(mem_pool, addr[4]);
    BoundTag_Display(mem_pool);
    BoundTag_Free(mem_pool, addr[3]);
    BoundTag_Display(mem_pool);
}