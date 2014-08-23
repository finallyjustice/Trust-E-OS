#include "tee_internal_api.h"
#include "tee_string.h"




// void* TEE_MemCopy(void* p_dst, void* p_src, uint32_t size)
// {
//     char *p_dst1 =(char*)p_dst;
//     char *p_src1 = (char*)p_src;
//     if(!p_dst || !p_src)
//         return NULL;
//     while(size--){
//         *p_dst1++ = *p_src1++;
//     }
//     return p_dst;
// }

// void TEE_MemSet(void* p_dst, char ch, uint32_t size)
// {
//     char *p_dst1 = p_dst;
//     if(!p_dst)
//         return;

//     while(size--){
//         *p_dst1++ = ch;
//     }
// }

uint32_t TEE_StrLen(const char* p_str)
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

char* TEE_StrCopy(char* dst, const char* src)
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

char* TEE_StrnCpy(char* dest, const char* src, uint32_t n)
{
	if (n != 0) {
		char *d = dest;
		const char *s = src;

		do {
			if ((*d++ = *s++) == 0) {
				/* NUL pad the remaining n-1 bytes */
				while (--n != 0)
					*d++ = 0;
				break;
			}
		} while (--n != 0);
	}
	return (dest);
}

int TEE_StrCmp (const char* s1, const char* s2)
{
  for(; *s1 == *s2; ++s1, ++s2)
    if(*s1 == 0)
      return 0;
  return *(unsigned char *)s1 < *(unsigned char *)s2 ? -1 : 1;
}