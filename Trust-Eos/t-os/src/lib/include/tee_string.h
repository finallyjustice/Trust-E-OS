#ifndef _LIB_TEE_STRING_H_
#define _LIB_TEE_STRING_H_

#include "tee_internal_api.h"

//void* TEE_MemCopy(void* p_dst, void* p_src, uint32_t size);
//void TEE_MemSet(void* p_dst, char ch, uint32_t size);
uint32_t TEE_StrLen(const char* p_str);
char* TEE_StrCopy(char* dst, const char* src);
char* TEE_StrnCpy(char* dest, const char* src, uint32_t n);
int TEE_StrCmp (const char* s1, const char* s2);

#endif