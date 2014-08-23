// TEE Memory Management Functions
#include "tee_mmf.h"
#include "tee_bound_tag.h"
#include "tee_string.h"
#include "tee_debug.h"

extern MEM_POOL g_mem_pool[];

// 说明：分配指定大小size字节的空间
// 参数：
// size:大小
// hint:默认0，其它值暂未加入
void* TEE_Malloc(size_t size, uint32_t hint)
{
    void *ret_addr = NULL;
    MEM_POOL *mem_pool = &g_mem_pool[0];
    ret_addr = BoundTag_Alloc(mem_pool, size);
    // 如果hint为0，则填充
    if (ret_addr && hint == 0) {
        TEE_MemFill(ret_addr, 0, size);
    }
    return ret_addr;
}

// 说明：改变所分配内存的大小
// 参数：
// buffer : 原始内存的起始地址
// new_size : 要重新分配的内存的大小
void *TEE_Realloc(in void* buffer, uint32_t new_size)
{
    void *new_buffer = NULL;
    uint32_t old_size = 0;
    if (!buffer) {
        return TEE_Malloc(new_size, 0);
    }

    // 检查buffer合法性
    if (!BoundTag_ValidAllocated(buffer)) {
        TEE_Printf("This is a programmer error! %d %d \n",__FILE__,__LINE__);
        while(1);
    }

    new_buffer = TEE_Malloc(new_size, 0);
    // 如果分配失败，返回NULL
    if (!new_buffer) {
        return NULL;
    }

    old_size = BoundTag_ValidSize(buffer);
    TEE_MemMove(new_buffer, buffer, old_size < new_size ? old_size : new_size);
    return new_buffer;
}


// 说明：将指定地址的内存块释放，以供于再分配
//       该函数会做地址合法性检查
void TEE_Free(void *buffer)
{
    MEM_POOL *mem_pool = NULL;
    if (!buffer) {
        return;
    }

    if (!BoundTag_ValidAllocated(buffer)) {
        TEE_Printf("This is a programmer error! %d %d \n",__FILE__,__LINE__);
        while(1);
    }

    mem_pool = &g_mem_pool[0];
    BoundTag_Free(mem_pool, buffer);
}

// 说明：buffer拷贝
void TEE_MemMove(out void *dest, in const void *src, uint32_t size)
{
    char *p_dst1 =(char*)dest;
    char *p_src1 = (char*)src;
    if(!p_dst1 || !p_src1) {
        TEE_Printf("This is a programmer error! FILE:%d LINE:%d\n",
                    __FILE__,__LINE__);
        while(1);
    }
         
    while(size--){
        *p_dst1++ = *p_src1++;
    }
}

// 说明：buffer比较
int32_t TEE_MemCompare(in const void *buffer1, in const void *buffer2, 
                        uint32_t size)
{
    int32_t i,j;
    char *buf1,*buf2;
    buf1 = (char *)buffer1;
    buf2 = (char *)buffer2;
    i = j = 0;
    while (i < size && j < size) {
        if (buf1[i] < buf2[j]) {
            return -1;
        }
        else if (buf1[i] > buf2[j]) {
            return 1;
        }
        i++;
        j++;
    }
    return 0;
}

// 说明：buffer填充
void TEE_MemFill(out void *buffer, uint32_t x,
                uint32_t size)
{
    int i;
    uint8_t *buf = (uint8_t*)buffer;
    for (i=0; i<size; i++) {
        buf[i] = (uint8_t)x;
    }
}


// 内存管理API测试函数（仅供测试）
void TEE_MMFTest()
{
    char *dest[100];
    int i = 0;
    MEM_POOL *mem_pool = &g_mem_pool[0];

    TEE_Printf("before allocating...\n");
    BoundTag_Display(mem_pool);
    TEE_Printf("\n====================MMF Test==================\n");

    for (i=0; i<100; i++) {
        dest[i] = TEE_Malloc(100, 0);
    }

    TEE_Printf("after allocating 100 blocks");
    BoundTag_Display(mem_pool);

    // 释放奇数块
    for (i=1; i<100; i+=2) {
        TEE_Free(dest[i]);
    }

    TEE_Printf("after delocating 1,3,5,...,99 block");
    BoundTag_Display(mem_pool);

    // 释放偶数块
    for (i=0; i<100; i+=2) {
        TEE_Free(dest[i]);
    }

    TEE_Printf("after delocating 2,4,8,...,100 block");
    BoundTag_Display(mem_pool);

    TEE_Printf("================================================\n");
}