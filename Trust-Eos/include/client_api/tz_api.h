#ifndef _TRUSTZONE_COMMUNICATION_TZ_CLIENT_API_H_
#define _TRUSTZONE_COMMUNICATION_TZ_CLIENT_API_H_

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/ioctl.h>

#include "driver/tz_driver.h"
#include "communication/services.h"
#include "communication/types.h"
#include "communication/constants.h"

#define TEEC_CONFIG_SHAREDMEM_MAX_SIZE  0x00080000
#define TEEC_ORIGIN_API                 0x00000001
#define TEEC_ORIGIN_COMMS               0x00000002
#define TEEC_ORIGIN_TEE                 0x00000003
#define TEEC_ORIGIN_TRUSTED_APP         0x00000004

// API Shared Memory Control Flags 
#define TEEC_MEM_INPUT                  0x00000001
#define TEEC_MEM_OUTPUT                 0x00000002

//API Parameter Types 
#define TEEC_NONE                       0x00000000
#define TEEC_VALUE_INPUT                0x00000001
#define TEEC_VALUE_OUTPUT               0x00000002
#define TEEC_VALUE_INOUT                0x00000003
#define TEEC_MEMREF_TEMP_INPUT          0x00000005
#define TEEC_MEMREF_TEMP_OUTPUT         0x00000006
#define TEEC_MEMREF_TEMP_INOUT          0x00000007
#define TEEC_MEMREF_WHOLE               0x0000000C
#define TEEC_MEMREF_PARTIAL_INPUT       0x0000000D
#define TEEC_MEMREF_PARTIAL_OUTPUT      0x0000000E
#define TEEC_MEMREF_PARTIAL_INOUT       0x0000000F

#define TEEC_PARAM_TYPES(t0, t1, t2, t3) \
                 ((t0) | ((t1) << 4) | ((t2) << 8) | ((t3) << 12))

#define TEEC_PARAM_TYPE_GET(t, i)    (((t) >> (i*4)) & 0xF)

// API Session Login Methods
#define TEEC_LOGIN_PUBLIC               0x00000000
#define TEEC_LOGIN_USER                 0x00000001
#define TEEC_LOGIN_GROUP                0x00000002
#define TEEC_LOGIN_APPLICATION          0x00000004
#define TEEC_LOGIN_USER_APPLICATION     0x00000005
#define TEEC_LOGIN_GROUP_APPLICATION    0x00000006

typedef struct {
    int fd;
}TEEC_Context;

typedef struct {
    TEEC_Context teec_context;
    SESSION_CONTEXT session_context;
}TEEC_Session; 

typedef struct {
    void*    buffer; 
    size_t   size; 
    uint32_t flags;
} TEEC_SharedMemory; 

typedef struct 
{ 
    void* buffer; 
    size_t size; 
} TEEC_TempMemoryReference;

typedef struct 
{ 
    TEEC_SharedMemory* parent; 
    size_t             size; 
    size_t             offset; 
} TEEC_RegisteredMemoryReference;

typedef struct 
{ 
   uint32_t a; 
   uint32_t b; 
} TEEC_Value;

typedef union 
{ 
    TEEC_TempMemoryReference       tmpref; 
    TEEC_RegisteredMemoryReference memref; 
    TEEC_Value                     value; 
} TEEC_Parameter;

typedef struct 
{ 
    uint32_t             started; 
    uint32_t             paramTypes; 
    TEEC_Parameter       params[4];  
} TEEC_Operation;

TEEC_Result TEEC_InitializeContext( 
    const char*   name, 
    TEEC_Context* context);

void TEEC_FinalizeContext( 
    TEEC_Context* context);

TEEC_Result TEEC_RegisterSharedMemory( 
    TEEC_Context*      context, 
    TEEC_SharedMemory* sharedMem);

TEEC_Result TEEC_AllocateSharedMemory( 
    TEEC_Context*      context, 
    TEEC_SharedMemory* sharedMem);

void TEEC_ReleaseSharedMemory ( 
    TEEC_SharedMemory* sharedMem);

TEEC_Result TEEC_OpenSession ( 
    TEEC_Context*    context, 
    TEEC_Session*    session, 
    const TEEC_UUID* destination, 
    uint32_t         connectionMethod, 
    const void*      connectionData, 
    TEEC_Operation*  operation, 
    uint32_t*        returnOrigin);

void TEEC_CloseSession ( 
    TEEC_Session* session);

TEEC_Result TEEC_InvokeCommand( 
    TEEC_Session*     session, 
    uint32_t          commandID, 
    TEEC_Operation*   operation, 
    uint32_t*         returnOrigin);

void TEEC_RequestCancellation(      
    TEEC_Operation* operation) ;

#endif
