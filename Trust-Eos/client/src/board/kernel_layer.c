#include "client_include.h"


// E3DD2D2C-E392-2F28-CE79-52EB78DE2500
static TEE_UUID client_uuid = {
                              0xE3DD2D2C,
                              0xE392,
                              0x2F28,
                              {0xCE, 0x79,
                               0x52, 0xEB,
                               0x78, 0xDE,
                               0x25, 0x00}
                                };

static int CallTZFunction(TZ_CMD_DATA *tz_cmd_data)
{
    volatile int ret = -1; 

    // in  : r0 = flag;
    // in  : r1 = &TZ_CMD_DATA
    // out : r0 = return value;
    register uint32_t r0 asm("r0") = SMC_ARG_TZ_API;
    register uint32_t r1 asm("r1") = (uint32_t)tz_cmd_data;
    r0 = r0;
    r1 = r1;
    asm volatile ("smc #0");
    // r0 is the return value
    ret = r0;
    return ret;
}

int OpenSession(user TEE_Param params[4],
                kernel out SESSION_CONTEXT *session_context,
                user uint32_t param_type)
{
    int ret = -1;
    TZ_CMD_DATA *tz_cmd_data = NULL;

    if (param_type != TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                                      TEE_PARAM_TYPE_NONE,
                                      TEE_PARAM_TYPE_NONE,
                                      TEE_PARAM_TYPE_NONE)) {
        return TEEC_ERROR_BAD_PARAMETERS;
    }
    tz_cmd_data = (TZ_CMD_DATA *)KernelMalloc(sizeof(TZ_CMD_DATA));
    // params[0] dest_uuid
    MemMove(&tz_cmd_data->params[0], &params[0], sizeof(TEE_Param));
    // params[1] client_uuid
    tz_cmd_data->params[1].memref.buffer = KernelMalloc(sizeof(TEE_UUID));
    tz_cmd_data->params[1].memref.size = sizeof(TEE_UUID);
    MemMove(tz_cmd_data->params[1].memref.buffer, &client_uuid, sizeof(TEE_UUID));
    // params[2] client login type
    tz_cmd_data->params[2].value.a = TEE_LOGIN_PUBLIC;
    tz_cmd_data->operation_type = TEE_OPERATION_OPEN_SESSION;

    param_type = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                                 TEE_PARAM_TYPE_MEMREF_INPUT,
                                 TEE_PARAM_TYPE_VALUE_INPUT,
                                 TEE_PARAM_TYPE_NONE);
    tz_cmd_data->param_type = param_type;
    tz_cmd_data->meta_data[0].usr_addr = params[0].memref.buffer;
    tz_cmd_data->meta_data[0].size = params[0].memref.size;
    ret = CallTZFunction(tz_cmd_data);
    MemMove(session_context, &tz_cmd_data->session_context, sizeof(SESSION_CONTEXT));
    KernelFree(tz_cmd_data);
    return ret;
}

int CloseSession(user SESSION_CONTEXT *session_context)
{
    TZ_CMD_DATA *tz_cmd_data = NULL;
    int ret = -1;

    tz_cmd_data = (TZ_CMD_DATA *)KernelMalloc(sizeof(TZ_CMD_DATA));
    Client_MemFill(tz_cmd_data, 0, sizeof(TZ_CMD_DATA));
    tz_cmd_data->operation_type = TEE_OPERATION_CLOSE_SESSION;
    MemMove(&tz_cmd_data->session_context, session_context, sizeof(SESSION_CONTEXT));
    ret = CallTZFunction(tz_cmd_data);
    return ret;//CallTZFunction(params, param_type, TEE_OPERATION_CLOSE_SESSION);
}

int InvokeSessionCommand(user SESSION_CONTEXT *session_context,
                        user TEE_Param params[4],
                        user uint32_t param_type,
                        int32_t command)
{
    TZ_CMD_DATA *tz_cmd_data = NULL;
    int ret = -1;
    tz_cmd_data = (TZ_CMD_DATA *)KernelMalloc(sizeof(TZ_CMD_DATA));
    Client_MemFill(tz_cmd_data, 0, sizeof(TZ_CMD_DATA));

    // params[0]
    tz_cmd_data->params[0].memref.buffer = KernelMalloc(params[0].memref.size);
    MemMove(tz_cmd_data->params[0].memref.buffer, params[0].memref.buffer, 
            params[0].memref.size);
    tz_cmd_data->params[0].memref.size = params[0].memref.size;
    // param_type
    tz_cmd_data->param_type = param_type;
    // operation_type
    tz_cmd_data->operation_type = TEE_OPERATION_INVOKE_COMMAND;
    // session_context
    MemMove(&tz_cmd_data->session_context, session_context, sizeof(SESSION_CONTEXT));
    // command id
    tz_cmd_data->command_id = command;
    // meta_data
    tz_cmd_data->meta_data[0].usr_addr = params[0].memref.buffer;
    tz_cmd_data->meta_data[0].size = params[0].memref.size;
    ret = CallTZFunction(tz_cmd_data);
    // write back
    MemMove(params[0].memref.buffer, tz_cmd_data->params[0].memref.buffer,
            params[0].memref.size);
    // free memory
    KernelFree(tz_cmd_data);


    return ret;
}