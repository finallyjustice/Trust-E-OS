#include "yaffsfs.h"

unsigned int sw_mem_malloc(unsigned int x);  //TEE_Malloc(x,0)
int sw_mem_free(unsigned int x);             //TEE_Free(x)
int sw_strcmp(const char *s1,const char *s2);   //TEE_StrCmp(s1,s2)
void *sw_memset(void * dest, unsigned int c, unsigned int count);  //TEE_MemFill(buff,x,size)
char *sw_strncpy(char * dest, char *src, int n);   //TEE_StrnCpy(s1,s2)
char *sw_strcpy(char * dest, char *src);           //TEE_StrCopy(s1,s2)
unsigned int sw_strlen(char * s);                 //TEE_StrLen(s)
int sw_memcmp(void *src, void *dest, unsigned int length);    //TEE_MemCompare(s1,s2,size)
void *sw_memcpy(void *dst, const void *src, unsigned int count);  //TEE_MemMove(d,s,size)
