// TEE Memory Management Functions
#ifndef _LIB_TEE_MMF_H_
#define _LIB_TEE_MMF_H_

#include "tee_internal_api.h"

// 说明：分配指定大小size字节的空间
// 参数：
// size:大小
// hint:默认0，其它值暂未加入
void* TEE_Malloc(size_t size, uint32_t hint);

// 说明：改变所分配内存的大小
// 参数：
// buffer : 原始内存的起始地址
// new_size : 要重新分配的内存的大小
void *TEE_Realloc(in void* buffer, uint32_t new_size);

// 说明：将指定地址的内存块释放，以供于再分配
//       该函数会做地址合法性检查
void TEE_Free(void *buffer);

// 说明：buffer拷贝
void TEE_MemMove(out void *dest, in const void *src, uint32_t size);

// 说明：buffer比较
int32_t TEE_MemCompare(in const void *buffer1, in const void *buffer2, uint32_t size);

// 说明：buffer填充
void TEE_MemFill(out void *buffer, uint32_t x, uint32_t size);

// 内存管理API测试函数（仅供测试）
void TEE_MMFTest();



#endif