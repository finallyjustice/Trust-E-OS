#include "tee_string_task.h" 

// GUID
// D1059173-97A7-ED77-619D-AB326DBA894B

TEE_Result TA_EXPORT String_CreateEntryPoint(void)
{ 
    TA_CreateEntryPoint();
    return TEE_SUCCESS;
}

void TA_EXPORT String_DestroyEntryPoint(void)
{
    TA_DestroyEntryPoint();
}

// params[0] memref_input 
TEE_Result TA_EXPORT String_OpenSessionEntryPoint(
                uint32_t    paramTypes,
        inout   TEE_Param   params[4],
        out ctx void**      sessionContext)
{
    return TA_OpenSessionEntryPoint(paramTypes, params,
                             sessionContext); 
}

void TA_EXPORT String_CloseSessionEntryPoint(
        ctx     void*       sessionContext)
{
    TA_CloseSessionEntryPoint(sessionContext);
}

TEE_Result TA_EXPORT String_InvokeCommandEntryPoint(
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

void Task_String()
{
    // static int32_t loop_cnt = 0;
    TEE_Param *params = NULL;
    uint32_t param_type = 0;
    int32_t command = -1;
    while(TRUE) {
        params = NULL;
        param_type = 0;
        command = -1;
        GetCurrentParams(&params, &param_type, &command);
        
        switch (command) {
            case STRING_LOWER_COMMAND:
                Task_StringLowerCommand(params, param_type);
                break;
            case STRING_UPPER_COMMAND:
                Task_StringUpperCommand(params, param_type);
                break;
            default:
                TEE_Printf("Not a valid string task command\n");
                TaskHandleReturn(TEE_ERROR_BAD_PARAMETERS);
                break;
        }
    }
}

static void StringLower(char *str, size_t str_len)
{
    size_t index;
    for (index = 0; index != str_len; index++) {
        if (str[index] >= 'A' && str[index] <= 'Z') {
            str[index] += 32;
        }
    }
}

// 参数要求，MEM_INOUT , NONE, NONE, NONE
void Task_StringLowerCommand(TEE_Param *params, uint32_t param_type)
{
    // check params
    char *buffer = NULL;
    if (param_type == TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT,
                                      TEE_PARAM_TYPE_NONE,
                                      TEE_PARAM_TYPE_NONE,
                                      TEE_PARAM_TYPE_NONE)) {

        buffer = (char *)map_from_ns((va_t)params[0].memref.buffer);
        StringLower(buffer, params[0].memref.size);
        TaskHandleReturn(TEE_SUCCESS);
    }
    else {
        TaskHandleReturn(TEE_ERROR_BAD_PARAMETERS);
    }
    
    // don't add any other codes here
    
}

static void StringUpper(char *str, size_t str_len)
{
    size_t index;
    for (index = 0; index != str_len; index++) {
        if (str[index] >= 'a' && str[index] <= 'z') {
            str[index] -= 32;
        }
    }
}

void Task_StringUpperCommand(TEE_Param *params, uint32_t param_type)
{
    char *buffer = NULL;
    // check params
    if (TEE_PARAM_TYPE_GET(param_type, 0) == TEE_PARAM_TYPE_MEMREF_INOUT
        && TEE_PARAM_TYPE_GET(param_type, 1) == TEE_PARAM_TYPE_NONE
        && TEE_PARAM_TYPE_GET(param_type, 2) == TEE_PARAM_TYPE_NONE
        && TEE_PARAM_TYPE_GET(param_type, 3) == TEE_PARAM_TYPE_NONE) {
        
         buffer = (char *)map_from_ns((va_t)params[0].memref.buffer);
        StringUpper(buffer, params[0].memref.size);
        TaskHandleReturn(TEE_SUCCESS);
    }
    else {
        TaskHandleReturn(TEE_ERROR_BAD_PARAMETERS);
    }
    // don't add any other codes here
}
