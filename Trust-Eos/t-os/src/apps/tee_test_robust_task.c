#include "tee_test_robust_task.h" 

// 该服务的功能，测试服务通信的健壮性
// 它与对应的客户端进行通信
TEE_Result TA_EXPORT TestRobust_CreateEntryPoint(void)
{
    TA_CreateEntryPoint();
    return TEE_SUCCESS;
}

void TA_EXPORT TestRobust_DestroyEntryPoint(void)
{
    TA_DestroyEntryPoint();
}

TEE_Result TA_EXPORT TestRobust_OpenSessionEntryPoint(
                uint32_t    paramTypes,
        inout   TEE_Param   params[4],
        out ctx void**      sessionContext)
{
    return TA_OpenSessionEntryPoint(paramTypes, params,
                             sessionContext); 
}

void TA_EXPORT TestRobust_CloseSessionEntryPoint(
        ctx     void*       sessionContext)
{
    TA_CloseSessionEntryPoint(sessionContext);
}

TEE_Result TA_EXPORT TestRobust_InvokeCommandEntryPoint(
        ctx     void*       sessionContext,
                uint32_t    commandID,
                uint32_t    paramTypes,
        inout   TEE_Param   params[4])
{
    return TA_InvokeCommandEntryPoint(sessionContext,
                                      commandID,
                                      paramTypes,
                                      params);
}

void Task_TestRobust(void)
{
    TEE_Param *params = NULL;
    uint32_t param_type = 0;
    int32_t command = -1;
    while(TRUE) {
        params = NULL;
        param_type = 0;
        command = -1;
        GetCurrentParams(&params, &param_type, &command);
        
        switch (command) {
            case TEST_ROBUST_COMMUNICATION_COMMAND:
                Task_TestRobustRunCommand(params, param_type);
                break;
            default:
                TEE_Printf("Not a valid test robust task command\n");
                TaskHandleReturn(TEE_ERROR_BAD_PARAMETERS);
                break;
        }
    }
}

void Task_TestRobustRunCommand(TEE_Param *params, uint32_t param_type)
{
    int i,t;
    char out_buffer[] = "I am robust";
    size_t out_buffer_len = TEE_StrLen(out_buffer) + 1;
    //uint32_t a,b;
    char *buffer;
    uint32_t size;

    for (i=0; i<4; i++) {
        t = TEE_PARAM_TYPE_GET(param_type, i);
        if (t == TEE_PARAM_TYPE_VALUE_INPUT) {
            // TEE_Printf("params[%d] TEE_PARAM_TYPE_VALUE_INPUT = 0x%x  0x%x\n",
            //             i, params[i].value.a, params[i].value.b);
        }
        else if (t == TEE_PARAM_TYPE_VALUE_OUTPUT) {
            // TEE_Printf("params[%d] TEE_PARAM_TYPE_VALUE_OUTPUT usr addr=0x%x 0x%x\n",
            //             i, params[i].value.a, params[i].value.b);
            params[i].value.a = 0xa;
            params[i].value.b = 0xb;
        }
        else if (t == TEE_PARAM_TYPE_VALUE_INOUT) {
            // TEE_Printf("params[%d] TEE_PARAM_TYPE_VALUE_INOUT = 0x%x 0x%x\n",
            //             i, params[i].value.a, params[i].value.b);
            params[i].value.a = 0xa;
            params[i].value.b = 0xb;
        }
        else if (t == TEE_PARAM_TYPE_MEMREF_INPUT) {
            buffer = (char *)map_from_ns((va_t)params[i].memref.buffer);
            size = params[i].memref.size;
            // TEE_Printf("params[%d] TEE_PARAM_TYPE_MEMREF_INPUT buffer=%s size=0x%x\n",
            //             i, buffer, size);
        }
        else if (t == TEE_PARAM_TYPE_MEMREF_OUTPUT) {
            buffer = (char *)map_from_ns((va_t)params[i].memref.buffer);
            size = params[i].memref.size;
            // TEE_Printf("params[%d] TEE_PARAM_TYPE_MEMREF_OUTPUT usr addr=0x%x 0x%x\n",
            //             i, (uint32_t)params[i].memref.buffer, size);
            if (out_buffer_len <= size) {
                TEE_MemMove(buffer, out_buffer, out_buffer_len);
                size = out_buffer_len;
            }
       }
       else if (t == TEE_PARAM_TYPE_MEMREF_INOUT) {
            buffer = (char *)map_from_ns((va_t)params[i].memref.buffer);
            size = params[i].memref.size;
            // TEE_Printf("params[%d] TEE_PARAM_TYPE_MEMREF_INOUT buffer=%s size=0x%x\n",
            //             i, buffer, size);
            if (out_buffer_len <= size) {
                TEE_MemMove(buffer, out_buffer, out_buffer_len);
                size = out_buffer_len;
            }
        }
        else {
            // TEE_Printf("params[%d] TEE_PARAM_TYPE_NONE\n", i);
        }
    }
    TaskHandleReturn(TEE_SUCCESS);
}


