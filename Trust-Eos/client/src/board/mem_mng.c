#include "client_include.h"

char user_mem[MEM_BLOCK_NUM][MEM_BLOCK_SIZE];

char kernel_mem[MEM_BLOCK_NUM][MEM_BLOCK_SIZE];

bool user_mem_flag[MEM_BLOCK_NUM];
bool kernel_mem_flag[MEM_BLOCK_NUM];

void MallocInit(void)
{
    int i,j;
    for (i=0; i<MEM_BLOCK_NUM; i++) {
        user_mem_flag[i] = FALSE;
        kernel_mem_flag[i] = FALSE;
    }
}

void *UserMalloc(size_t size)
{
    int i;
    if (size > MEM_BLOCK_SIZE) {
        return NULL;
    }
    for (i = 0; i<MEM_BLOCK_NUM; i++) {
        if (user_mem_flag[i] == FALSE) {
            user_mem_flag[i] = TRUE;
            return (void *)user_mem[i];
        }
    }

    return NULL;
}

void *KernelMalloc(size_t size)
{
    int i;
    if (size > MEM_BLOCK_SIZE) {
        return NULL;
    }
    for (i = 0; i<MEM_BLOCK_NUM; i++) {
        if (kernel_mem_flag[i] == FALSE) {
            kernel_mem_flag[i] = TRUE;
            return (void *)kernel_mem[i];
        }
    }
}


void UserFree(void *addr)
{
    int i;
    for (i=0; i<MEM_BLOCK_NUM; i++) {
        if ((uint32_t)addr == (uint32_t)user_mem[i]) {
            user_mem_flag[i] = FALSE;
            break;
        }
    }
}

void KernelFree(void *addr)
{
    int i;
    for (i=0; i<MEM_BLOCK_NUM; i++) {
        if ((uint32_t)addr == (uint32_t)kernel_mem[i]) {
            kernel_mem_flag[i] = FALSE;
            break;
        }
    }
}


void MemMove(void *dest, void *src, uint32_t size)
{
    char *p_dst1 =(char*)dest;
    char *p_src1 = (char*)src;
    if(!p_dst1 || !p_src1) {
        Client_Printf("This is a programmer error! FILE:%s LINE:%d\n",
                    __FILE__,__LINE__);
        while(1);
    }
         
    while(size--){
        *p_dst1++ = *p_src1++;
    }
}


uint32_t Client_StrLen(char* p_str)
{
    uint32_t len = 0;
    if(!p_str){
        return 0;
    }

    while(*p_str++ != '\0'){
        len++;
    }
    return len;
}

char* Client_StrCopy(char* dst, const char* src)
{
    char *address = dst;
    if(!dst || !src){
        return NULL;
    }

    while(*src != '\0'){
        *address++ = *src++;
    }
    *address = '\0';
    return dst;
}

void Client_MemFill(out void *buffer, uint32_t x,
                uint32_t size)
{
    int i;
    uint8_t *buf = (uint8_t*)buffer;
    for (i=0; i<size; i++) {
        buf[i] = (uint8_t)x;
    }
}