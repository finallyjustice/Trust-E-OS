#ifndef _CLIENT_INCLUDE_H_
#define _CLIENT_INCLUDE_H_

#define SMC_ARG_TZ_API                      (0x80000001)

#define user
#define kernel
#define in
#define out
#define inout

typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned char uint8_t;
typedef char int8_t;
typedef unsigned int bool;
typedef unsigned long long uint64_t;
typedef long long int64_t;
typedef uint32_t size_t;
typedef uint32_t   TEE_Result;
typedef TEE_Result  TEEC_Result;
#define TRUE 1
#define FALSE 0
#define NULL 0
#define MEM_BLOCK_NUM    64
#define MEM_BLOCK_SIZE   1024

// enum TEE_OPEARTION_TYPE {
//     TEE_OPERATION_OPEN_SESSION = 0x80000000,
//     TEE_OPERATION_CLOSE_SESSION,
//     TEE_OPERATION_INVOKE_COMMAND
// };

// 某些交互的声明
// enum TEE_PARAM_TYPE {
//     TEE_PARAM_TYPE_NONE = 0, 
//     TEE_PARAM_TYPE_VALUE_INPUT = 1,
//     TEE_PARAM_TYPE_VALUE_OUTPUT = 2,
//     TEE_PARAM_TYPE_VALUE_INOUT = 3,
//     TEE_PARAM_TYPE_MEMREF_INPUT = 5,
//     TEE_PARAM_TYPE_MEMREF_OUTPUT = 6, 
//     TEE_PARAM_TYPE_MEMREF_INOUT = 7,
// };

// typedef struct 
// {  
//     uint32_t timeLow;  
//     uint16_t timeMid;  
//     uint16_t timeHiAndVersion; 
//     uint8_t  clockSeqAndNode[8]; 
// }TEE_UUID;

//typedef TEE_UUID TEEC_UUID;

// Oeration Parameters in the TA Interface
// #define TEE_PARAM_TYPES(t0, t1, t2, t3) \
//                  ((t0) | ((t1) << 4) | ((t2) << 8) | ((t3) << 12))

// #define TEE_PARAM_TYPE_GET(t, i)    (((t) >> (i*4)) & 0xF)





typedef struct _PARAM_META_DATA {
    void *usr_addr;
    size_t size;
}PARAM_META_DATA;







#define TEE_PARAM_TYPES(t0, t1, t2, t3) \
                 ((t0) | ((t1) << 4) | ((t2) << 8) | ((t3) << 12))

#define TEE_PARAM_TYPE_GET(t, i)    (((t) >> (i*4)) & 0xF)

void *UserMalloc(size_t size);

void *KernelMalloc(size_t size);

void MallocInit(void);

//nt CallTZFunction(user TEE_Param param[4],user uint32_t param_type);

// 说明：buffer拷贝
void MemMove(void *dest, void *src, uint32_t size);

void UserFree(void *addr);

void KernelFree(void *addr);


int OpenSession(user TEE_Param params[4],
                kernel out SESSION_CONTEXT *session_context,
                user uint32_t param_type);

int CloseSession(user SESSION_CONTEXT *session_context);

int InvokeSessionCommand(user SESSION_CONTEXT *session_context,
                        user TEE_Param params[4],
                        user uint32_t param_type,
                        int32_t command);

uint32_t Client_StrLen(char* p_str);

char* Client_StrCopy(char* dst, const char* src);

// 说明：buffer填充
void Client_MemFill(out void *buffer, uint32_t x, uint32_t size);

#define TEE_SUCCESS                 (0x00000000)
#define TEEC_SUCCESS                (0x00000000)
#define TEE_ERROR_GENERIC           (0xFFFF0000) 
#define TEEC_ERROR_GENERIC          (0xFFFF0000)
#define TEE_ERROR_ACCESS_DENIED     (0xFFFF0001)
#define TEEC_ERROR_ACCESS_DENIED    (0xFFFF0001)
#define TEE_ERROR_CANCEL            (0xFFFF0002)
#define TEEC_ERROR_CANCEL           (0xFFFF0002)
#define TEE_ERROR_ACCESS_CONFLICT   (0xFFFF0003)
#define TEEC_ERROR_ACCESS_CONFLICT  (0xFFFF0003)
#define TEE_ERROR_EXCESS_DATA       (0xFFFF0004)
#define TEEC_ERROR_EXCESS_DATA      (0xFFFF0004)
#define TEE_ERROR_BAD_FORMAT        (0xFFFF0005)
#define TEEC_ERROR_BAD_FORMAT       (0xFFFF0005)
#define TEE_ERROR_BAD_PARAMETERS    (0xFFFF0006)
#define TEEC_ERROR_BAD_PARAMETERS   (0xFFFF0006)
#define TEE_ERROR_BAD_STATE         (0xFFFF0007)
#define TEEC_ERROR_BAD_STATE        (0xFFFF0007)
#define TEE_ERROR_ITEM_NOT_FOUND    (0xFFFF0008)
#define TEEC_ERROR_ITEM_NOT_FOUND   (0xFFFF0008)
#define TEE_ERROR_NOT_IMPLEMENTED   (0xFFFF0009)
#define TEEC_ERROR_NOT_IMPLEMENTED  (0xFFFF0009)
#define TEE_ERROR_NOT_SUPPORTED     (0xFFFF000A)
#define TEEC_ERROR_NOT_SUPPORTED    (0xFFFF000A)
#define TEE_ERROR_NO_DATA           (0xFFFF000B)
#define TEEC_ERROR_NO_DATA          (0xFFFF000B)
#define TEE_ERROR_OUT_OF_MEMORY     (0xFFFF000C)
#define TEEC_ERROR_OUT_OF_MEMORY    (0xFFFF000C)
#define TEE_ERROR_BUSY              (0xFFFF000D)
#define TEEC_ERROR_BUSY             (0xFFFF000D)
#define TEE_ERROR_COMMUNICATION     (0xFFFF000E)
#define TEEC_ERROR_COMMUNICATION    (0xFFFF000E)
#define TEE_ERROR_SECURITY          (0xFFFF000F)
#define TEEC_ERROR_SECURITY         (0xFFFF000F)
#define TEE_ERROR_SHORT_BUFFER      (0xFFFF0010)
#define TEEC_ERROR_SHORT_BUFFER     (0xFFFF0010)
#define TEE_PENDING                 (0xFFFF2000)
#define TEE_ERROR_TIMEOUT           (0xFFFF3001)
#define TEE_ERROR_OVERFLOW          (0xFFFF300F)
#define TEE_ERROR_TARGET_DEAD       (0xFFFF3024)
#define TEEC_ERROR_TARGET_DEAD      (0xFFFF3024)
#define TEE_ERROR_STORAGE_NO_SPACE  (0xFFFF3041)
#define TEE_ERROR_MAC_INVALID       (0xFFFF3071)
#define TEE_ERROR_SIGNATURE_INVALID (0xFFFF3072)
#define TEE_ERROR_TIME_NOT_SET      (0xFFFF5000)
#define TEE_ERROR_TIME_NEEDS_RESET  (0xFFFF5001)

enum TEE_LOGIN {
    TEE_LOGIN_PUBLIC = 0x00000000,
    TEE_LOGIN_USER = 0x00000001,
    TEE_LOGIN_GROUP = 0x00000002,
    TEE_LOGIN_APPLICATION = 0x00000004,
    TEE_LOGIN_APPLICATION_USER = 0x00000005,
    TEE_LOGIN_APPLICATION_GROUP = 0x00000006,
    TEE_LOGIN_TRUSTED_APP = 0xF0000000,
};

#endif