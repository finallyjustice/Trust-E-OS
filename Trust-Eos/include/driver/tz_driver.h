#ifndef _TRUSTZONE_COMMUNICATION_DRIVER_H_
#define _TRUSTZONE_COMMUNICATION_DRIVER_H_


#include <linux/ioctl.h> 


#include "communication/types.h"

#define TZ_MAGIC         'T'
#define TZ_MAX_NR         3     // 最大命令叙述

/* IOCTL的操作有
 * 1. Open Session
 * 2. Close Session
 * 3. Invoke Session
 */

#define TZ_IOCTL_OPEN_SESSION       _IOWR(TZ_MAGIC, 1, OPEN_SESSION_DATA)
#define TZ_IOCTL_CLOSE_SESSION      _IOWR(TZ_MAGIC, 2, CLOSE_SESSION_DATA)
#define TZ_IOCTL_INVOKE_SESSION     _IOWR(TZ_MAGIC, 3, INVOKE_SESSION_DATA)

typedef struct 
{ 
    unsigned int started; 
    unsigned int param_type; 
    TEEC_Param params[4];
    // TEEC_Param *params;
    //<Implementation-Defined Type> imp; 
} TZ_Operation; 


typedef struct _OPEN_SESSION_DATA{
    TZ_Operation *tz_operation;
    SESSION_CONTEXT *session_context;
    unsigned int *return_origin;
}OPEN_SESSION_DATA;

typedef struct _CLOSE_SESSION_DATA {
    SESSION_CONTEXT *session_context;
}CLOSE_SESSION_DATA;

typedef struct _INVOKE_SESSION_DATA {
    TZ_Operation *tz_operation;
    SESSION_CONTEXT *session_context;
    int command;
    uint32_t *return_origin;
}INVOKE_SESSION_DATA;


#endif