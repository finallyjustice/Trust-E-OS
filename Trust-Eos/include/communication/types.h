#ifndef _TRUSTZONE_COMMUNICATION_TYPES_H_
#define _TRUSTZONE_COMMUNICATION_TYPES_H_


typedef unsigned int   TEEC_Result;

typedef struct 
{  
    unsigned int timeLow;  
    unsigned short timeMid;  
    unsigned short timeHiAndVersion; 
    unsigned char  clockSeqAndNode[8]; 
}TEEC_UUID;

enum TEE_PARAM_TYPE {
    TEE_PARAM_TYPE_NONE = 0, 
    TEE_PARAM_TYPE_VALUE_INPUT = 1,
    TEE_PARAM_TYPE_VALUE_OUTPUT = 2,
    TEE_PARAM_TYPE_VALUE_INOUT = 3,
    TEE_PARAM_TYPE_MEMREF_INPUT = 5,
    TEE_PARAM_TYPE_MEMREF_OUTPUT = 6, 
    TEE_PARAM_TYPE_MEMREF_INOUT = 7,
};

#define TEE_PARAM_TYPES(t0, t1, t2, t3) \
                 ((t0) | ((t1) << 4) | ((t2) << 8) | ((t3) << 12))

#define TEE_PARAM_TYPE_GET(t, i)    (((t) >> (i*4)) & 0xF)

enum TEE_OPEARTION_TYPE {
    TEE_OPERATION_OPEN_SESSION = 0x80000000,
    TEE_OPERATION_CLOSE_SESSION,
    TEE_OPERATION_INVOKE_COMMAND
};

typedef union  
{  
    struct  
    {  
       void *buffer; 
       unsigned int size;
    } memref; 
    struct  
    {  
       unsigned int a;
       unsigned int b;
    } value;  
}TEEC_Param; 

typedef struct _SESSION_CONTEXT {
    TEEC_UUID client_uuid;
    int session_id;
}SESSION_CONTEXT;


typedef struct _TZ_CMD_DATA {
    // 参数
    TEEC_Param params[4];
    // 参数类型
    unsigned int param_type;
    // 目标服务标识
    // TEE_UUID dest_uuid;
    // 操作类型：
    // 打开会话 - TEE_OPERATION_OPEN_SESSION，
    // 关闭会话 - TEE_OPERATION_CLOSE_SESSION,
    // 执行命令 - TEE_OPERATION_INVOKE_COMMAND
    enum TEE_OPEARTION_TYPE operation_type;
    // 会话ID 
    // （在操作类型为TEE_OPERATION_OPEN_SESSION的时候作为返回值，
    //  在操作类型为TEE_OPERATION_INVOKE_COMMAND的时候作为参数值）
    SESSION_CONTEXT session_context;
    // 操作命令 (在操作类型为TEE_OPERATION_INVOKE_COMMAND的时候所用)
    int command_id;
    // 标识在T-OS中执行TZ命令时候的返回位置
    int return_origin;
    // 参数辅助元数据，用于标识参数在用户空间的地址及大小
    // PARAM_META_DATA meta_data[4];
}TZ_CMD_DATA;


#endif