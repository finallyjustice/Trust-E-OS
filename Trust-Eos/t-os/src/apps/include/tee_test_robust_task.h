#ifndef _APPS_TEE_Task_TEST_ROBUST_H_
#define _APPS_TEE_Task_TEST_ROBUST_H_

#include "tee_core_api.h"

TEE_Result TA_EXPORT TestRobust_CreateEntryPoint(void); 

void TA_EXPORT TestRobust_DestroyEntryPoint(void); 

TEE_Result TA_EXPORT TestRobust_OpenSessionEntryPoint(
                uint32_t    paramTypes,
        inout   TEE_Param   params[4],
        out ctx void**      sessionContext);

void TA_EXPORT TestRobust_CloseSessionEntryPoint(
        ctx     void*       sessionContext);

TEE_Result TA_EXPORT TestRobust_InvokeCommandEntryPoint(
        ctx     void*       sessionContext,
                uint32_t    commandID,
                uint32_t    paramTypes,
        inout   TEE_Param   params[4]);

void Task_TestRobust(void);

void Task_TestRobustRunCommand(TEE_Param *params, uint32_t param_type);


#endif // tee_test_robust_task.h