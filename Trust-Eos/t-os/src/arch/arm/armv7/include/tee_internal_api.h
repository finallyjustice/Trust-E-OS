#ifndef ARM_ARMV7_TEE_INTERNAL_API_H
#define ARM_ARMV7_TEE_INTERNAL_API_H

#include "communication/types.h"
#include "communication/services.h"
#include "communication/constants.h"

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


typedef TEEC_Result TEE_Result;

typedef TEEC_UUID TEE_UUID;

typedef TEEC_Param TEE_Param;

    // Error Codes
#define TEE_SUCCESS                 (0x00000000)
#define TEE_ERROR_GENERIC           (0xFFFF0000) 
#define TEE_ERROR_ACCESS_DENIED     (0xFFFF0001)
#define TEE_ERROR_CANCEL            (0xFFFF0002)
#define TEE_ERROR_ACCESS_CONFLICT   (0xFFFF0003)
#define TEE_ERROR_EXCESS_DATA       (0xFFFF0004)
#define TEE_ERROR_BAD_FORMAT        (0xFFFF0005)
#define TEE_ERROR_BAD_PARAMETERS    (0xFFFF0006)
#define TEE_ERROR_BAD_STATE         (0xFFFF0007)
#define TEE_ERROR_ITEM_NOT_FOUND    (0xFFFF0008)
#define TEE_ERROR_NOT_IMPLEMENTED   (0xFFFF0009)
#define TEE_ERROR_NOT_SUPPORTED     (0xFFFF000A)
#define TEE_ERROR_NO_DATA           (0xFFFF000B)
#define TEE_ERROR_OUT_OF_MEMORY     (0xFFFF000C)
#define TEE_ERROR_BUSY              (0xFFFF000D)
#define TEE_ERROR_COMMUNICATION     (0xFFFF000E)
#define TEE_ERROR_SECURITY          (0xFFFF000F)
#define TEE_ERROR_SHORT_BUFFER      (0xFFFF0010)
#define TEE_PENDING                 (0xFFFF2000)
#define TEE_ERROR_TIMEOUT           (0xFFFF3001)
#define TEE_ERROR_OVERFLOW          (0xFFFF300F)
#define TEE_ERROR_TARGET_DEAD       (0xFFFF3024)
#define TEE_ERROR_STORAGE_NO_SPACE  (0xFFFF3041)
#define TEE_ERROR_MAC_INVALID       (0xFFFF3071)
#define TEE_ERROR_SIGNATURE_INVALID (0xFFFF3072)
#define TEE_ERROR_TIME_NOT_SET      (0xFFFF5000)
#define TEE_ERROR_TIME_NEEDS_RESET  (0xFFFF5001)

// Parameter Annotations
#define out      
#define in
#define inout
#define outopt
#define inbuf
#define outbuf
#define outbufopt
#define instring
#define instringopt
#define outstring
#define outstringopt
#define ctx
 


#define FALSE   0
#define TRUE    1
#define NULL    0




// 4.1.1 TEE_Identity
typedef struct 
{  
    // TEE_LOGIN_XXX
    uint32_t    login;
    // client UUID or Nil
    TEE_UUID    uuid;
}TEE_Identity; 


// TA_OpenSessionEntryPoint，TA_InvokeCommandEntryPoint,
// TEE_OpenTASession, TEE_InvokeTACommand调用时候使用

// 4.1.3 TEE_TASessionHandle
typedef uint32_t __TEE_TASessionHandle;
typedef /*struct*/ __TEE_TASessionHandle* TEE_TASessionHandle;

// 4.1.4 TEE_PropSetHandle
typedef uint32_t __TEE_PropSetHandle;
typedef struct __TEE_PropSetHandle* TEE_PropSetHandle;

// 4.2.1 Parameter Types


// 4.2.2 Login Types
enum TEE_LOGIN {
    TEE_LOGIN_PUBLIC = 0x00000000,
    TEE_LOGIN_USER = 0x00000001,
    TEE_LOGIN_GROUP = 0x00000002,
    TEE_LOGIN_APPLICATION = 0x00000004,
    TEE_LOGIN_APPLICATION_USER = 0x00000005,
    TEE_LOGIN_APPLICATION_GROUP = 0x00000006,
    TEE_LOGIN_TRUSTED_APP = 0xF0000000,
};

// 4.2.3 Origin Codes
enum TEE_ORIGIN {
    TEE_ORIGIN_API = 0x00000001,
    TEE_ORIGIN_COMMS = 0x00000002,
    TEE_ORIGIN_TEE = 0x00000003,
    TEE_ORIGIN_TRUSTED_APP = 0x00000004,
};


// 4.2.4 Property Set Pseudo-Handles
#define TEE_PROPSET_CURRENT_TA          (0xFFFFFFFF)
#define TEE_PROPSET_CURRENT_CLIENT      (0xFFFFFFFE)
#define TEE_PROPSET_TEE_IMPLEMENTATION  (0xFFFFFFFD)

// 4.2.4 Memory Access Rights
#define TEE_ACCESS_READ         (0x00000001)
#define TEE_ACCESS_WRITE        (0x00000002)
#define TEE_ACCESS_ANY_OWNER    (0x00000004)


#endif