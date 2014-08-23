
#include "client_api/tz_api.h"
#include "communication/types.h"
#include "communication/services.h"
#include "communication/constants.h"

//static TEEC_UUID string_uuid = TASK_STRING_UUID;

static char test_string[] = "Hello TrustZone Driver";

static TEEC_Result TZInvokeCommand(int fd,
                            SESSION_CONTEXT *session,
                            uint32_t command,
                            TZ_Operation *operation,
                            uint32_t *return_origin)
{
    int i = 0;
    int ret = 0;
    INVOKE_SESSION_DATA invoke_session_data;
    //TZ_Operation dest_operation;
    if (session == NULL || operation == NULL) {
        return TEEC_ERROR_BAD_PARAMETERS;
    }
    
    invoke_session_data.session_context = session;
    invoke_session_data.command = command;
    invoke_session_data.tz_operation = operation;
    invoke_session_data.return_origin = return_origin;
    ret = ioctl(fd, TZ_IOCTL_INVOKE_SESSION, &invoke_session_data);
    return ret;
}

static void TZCloseSession(int fd, SESSION_CONTEXT* session)
{
    int ret = 0;
    CLOSE_SESSION_DATA close_session_data;
    close_session_data.session_context = session;
    ret = ioctl(fd, TZ_IOCTL_CLOSE_SESSION, &close_session_data);
    if (TEEC_SUCCESS == ret) {
        printf("Close session successfully!\n");
    }
    else {
        printf("Close session failed!\n");
    }
}
/*
 * 说明：对于在对外API的所有函数中，其中所使用的TEE_Param的int
 *       型数据都被当做指针使用，其目的是方便一些INOUT参数在内核
 *       驱动中写回用户空间（copy_to_user函数使用的指针操作),
 *       虽然这样写在一定程度上会降低程序的可读性，但是为了保证
 *       API的实现全部一致，不管该参数类型是IN,OUT还是INOUT，我们都
 *       将所有TEE_Param的字段（memref.buffer,memref.size,value.a,
 *       value.b)都转换为指针uint32_t* 使用。
 */
static TEEC_Result TZOpenSession(int fd, 
                          SESSION_CONTEXT *session,
                          const TEEC_UUID *destination,
                          uint32_t conn_method,
                          const void* conn_data,
                          TZ_Operation* operation,
                          uint32_t *return_origin)
{
    // 目前我规定中在OpenSession的时候，传入operation应该为空。
    // 在调用驱动的时候需要让params[0]为目标UUID,类型为MEMREF_INPUT
    int ret = 0;
    OPEN_SESSION_DATA open_session_data;
    TZ_Operation tz_operation;
    memset(&tz_operation, 0, sizeof(TZ_Operation));

    tz_operation.params[0].memref.buffer = malloc(sizeof(TEEC_UUID)); 
    memcpy(tz_operation.params[0].memref.buffer, destination, sizeof(TEEC_UUID));
    tz_operation.params[0].memref.size = (uint32_t)malloc(sizeof(uint32_t)); 
    *(uint32_t *)tz_operation.params[0].memref.size = sizeof(TEEC_UUID);
    tz_operation.param_type = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                                                TEE_PARAM_TYPE_NONE,
                                                TEE_PARAM_TYPE_NONE,
                                                TEE_PARAM_TYPE_NONE);
    open_session_data.tz_operation = &tz_operation;
    open_session_data.session_context = session;
    open_session_data.return_origin = return_origin;
    ret = ioctl(fd, TZ_IOCTL_OPEN_SESSION, &open_session_data);
    
    free((void *)tz_operation.params[0].memref.size);
    free(tz_operation.params[0].memref.buffer);

    return ret;
}

// 将TEEC_Operation 转换为 TZ_Operation
static void OperationConvert(TZ_Operation *tz_operation, TEEC_Operation *teec_operation)
{
    int i=0;
    int types[4];
    int t;
    for (i=0; i<4; i++) {
        t = TEEC_PARAM_TYPE_GET(teec_operation->paramTypes,i);

        if (t == TEEC_VALUE_INPUT || t == TEEC_VALUE_OUTPUT || t == TEEC_VALUE_INOUT ) {
            tz_operation->params[i].value.a = (uint32_t)&teec_operation->params[i].value.a;
            tz_operation->params[i].value.b = (uint32_t)&teec_operation->params[i].value.b;
            if (t == TEEC_VALUE_INPUT) 
                {  types[i] = TEE_PARAM_TYPE_VALUE_INPUT; }
            else if (t == TEEC_VALUE_OUTPUT)
                {  types[i] = TEE_PARAM_TYPE_VALUE_OUTPUT; }
            else 
                {  types[i] = TEE_PARAM_TYPE_VALUE_INOUT; }
        }
        else if (t == TEEC_MEMREF_TEMP_INPUT 
                || t == TEEC_MEMREF_TEMP_OUTPUT 
                || t == TEEC_MEMREF_TEMP_INOUT) {
            tz_operation->params[i].memref.buffer = teec_operation->params[i].tmpref.buffer;
            tz_operation->params[i].memref.size = (uint32_t)&teec_operation->params[i].tmpref.size;
            if (t == TEEC_MEMREF_TEMP_INPUT) {
                types[i] = TEE_PARAM_TYPE_MEMREF_INPUT;} 
            else if (t == TEEC_MEMREF_TEMP_OUTPUT) {
                types[i] = TEE_PARAM_TYPE_MEMREF_OUTPUT;}
            else {
                types[i] = TEE_PARAM_TYPE_MEMREF_INOUT;}
        }
        else if (t == TEEC_MEMREF_WHOLE 
                || t == TEEC_MEMREF_PARTIAL_INPUT 
                || t == TEEC_MEMREF_PARTIAL_OUTPUT
                || t == TEEC_MEMREF_PARTIAL_INOUT) {
            tz_operation->params[i].memref.buffer = teec_operation->params[i].memref.parent->buffer + 
                                                    teec_operation->params[i].memref.offset;
            tz_operation->params[i].memref.size = (uint32_t)&teec_operation->params[i].memref.size;
            if (t == TEEC_MEMREF_PARTIAL_INPUT) {
                types[i] = TEE_PARAM_TYPE_MEMREF_INPUT;}
            else if (t == TEEC_MEMREF_PARTIAL_OUTPUT) {
                types[i] = TEE_PARAM_TYPE_MEMREF_OUTPUT;}
            else {
                types[i] = TEE_PARAM_TYPE_MEMREF_INOUT;}
        }
        else {
            types[i] = TEE_PARAM_TYPE_NONE;
        }
    }
    tz_operation->param_type = TEE_PARAM_TYPES(types[0], types[1], types[2], types[3]);
}

TEEC_Result TEEC_InitializeContext( 
    const char*   name, 
    TEEC_Context* context)
{
    if (context == NULL) {
        return TEEC_ERROR_BAD_PARAMETERS;
    }
    context->fd = open(name, O_RDWR);
    if (-1 == context->fd) {
        return TEEC_ERROR_GENERIC;
    }
    return TEEC_SUCCESS;
}

void TEEC_FinalizeContext( 
    TEEC_Context* context)
{
    if (context) {
        if (0 != close(context->fd)) {
            printf("[Client API Info:] TEEC_FinalizeContext Failed!\n");
        }
    }
}

TEEC_Result TEEC_RegisterSharedMemory( 
    TEEC_Context*      context, 
    TEEC_SharedMemory* sharedMem)
{
    return TEEC_SUCCESS;
}

TEEC_Result TEEC_AllocateSharedMemory( 
    TEEC_Context*      context, 
    TEEC_SharedMemory* sharedMem)
{
    if(sharedMem && sharedMem->size > 0) {
        sharedMem->buffer = malloc(sharedMem->size);
        if (sharedMem->buffer == NULL) {
            return TEEC_ERROR_OUT_OF_MEMORY;
        }
        return TEEC_SUCCESS;
    }
    return TEEC_ERROR_GENERIC;
}

void TEEC_ReleaseSharedMemory ( 
    TEEC_SharedMemory* sharedMem)
{
    if (sharedMem && sharedMem->buffer) {
        free(sharedMem->buffer);
        sharedMem->buffer = NULL;
    }
}

void TEEC_CloseSession ( 
    TEEC_Session* session)
{
    if (session) {
        TZCloseSession(session->teec_context.fd,
                       &session->session_context);
    }
}
TEEC_Result TEEC_OpenSession ( 
    TEEC_Context*    context, 
    TEEC_Session*    session, 
    const TEEC_UUID* destination, 
    uint32_t         connectionMethod, 
    const void*      connectionData, 
    TEEC_Operation*  teec_operation, 
    uint32_t*        returnOrigin)
{
    TZ_Operation tz_operation;
    if (context == NULL ||session == NULL ||
        destination == NULL) {
        return TEEC_ERROR_BAD_PARAMETERS;
    }
    session->teec_context = *context;
    // Open的时候我不需要operation参数
    return TZOpenSession(context->fd, &session->session_context,
                  destination, connectionMethod,
                  connectionData, NULL, returnOrigin);

}

TEEC_Result TEEC_InvokeCommand( 
    TEEC_Session*     session, 
    uint32_t          commandID, 
    TEEC_Operation*   operation, 
    uint32_t*         returnOrigin)
{
    TZ_Operation tz_operation;
    if(operation) {
        OperationConvert(&tz_operation, operation);
    }
    return TZInvokeCommand(session->teec_context.fd,
                           &session->session_context,
                           commandID,
                           &tz_operation,
                           returnOrigin);
}

