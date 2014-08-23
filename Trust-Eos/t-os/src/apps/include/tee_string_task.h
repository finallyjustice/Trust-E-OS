#ifndef _APPS_TEE_Task_String_H_
#define _APPS_TEE_Task_String_H_

#include "tee_core_api.h"

TEE_Result TA_EXPORT String_CreateEntryPoint(void);


void TA_EXPORT String_DestroyEntryPoint(void);


TEE_Result TA_EXPORT String_OpenSessionEntryPoint(
                uint32_t    paramTypes,
        inout   TEE_Param   params[4],
        out ctx void**      sessionContext);

void TA_EXPORT String_CloseSessionEntryPoint(
        ctx     void*       sessionContext);

TEE_Result TA_EXPORT String_InvokeCommandEntryPoint(
        ctx     void*       sessionContext,
                uint32_t    commandID,
                uint32_t    paramTypes,
        inout   TEE_Param   params[4]);

void Task_String(void);

void Task_StringLowerCommand(TEE_Param *params, uint32_t param_type);

void Task_StringUpperCommand(TEE_Param *params, uint32_t param_type);


#endif // tee_Task_String.h