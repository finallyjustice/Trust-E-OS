#include "client_api/tz_api.h"

static TEEC_UUID string_task_uuid = TASK_STRING_UUID;


int main()
{
    printf("=======================Hello Android=====================\n");
    printf("Current is nosecure state!\n");
    char name[] ="/dev/trustzone_driver";
    char test_string[] = "hello WORLD";
    TEEC_Context context;
    TEEC_Session session;
    TEEC_Operation operation;
    uint32_t return_origin;

    // 使用共享内存的方法
    TEEC_SharedMemory shared_mem;
    shared_mem.buffer = test_string;
    shared_mem.size = strlen(test_string) + 1;
    shared_mem.flags = 0;
    operation.params[0].memref.parent = &shared_mem;
    operation.params[0].memref.size = shared_mem.size;
    operation.params[0].memref.offset = 0;
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE,
                                            TEEC_NONE, TEEC_NONE, TEEC_NONE);

    if (TEEC_SUCCESS != TEEC_InitializeContext(name, &context)) {
        printf("[API INFO:] TEEC_InitializeContext failed \n");
        fflush(stdout);
        return 0;
    }

    if (TEEC_SUCCESS != TEEC_OpenSession(&context, &session, &string_task_uuid, 0, NULL, NULL, &return_origin)) {
        printf("[API INFO:] TEEC_OpenSession failed \n");
        return 0;
    }
    printf("[API INFO:] open session ok, session_id = 0x%x \n", session.session_context.session_id);
    fflush(stdout);

    if (TEEC_SUCCESS == TEEC_InvokeCommand(&session, STRING_LOWER_COMMAND, 
                                        &operation, &return_origin)) {
       printf("[API INFO:]  STRING_LOWER_COMMAND :%s \n", test_string);
        fflush(stdout);
    }
    

    if (TEEC_SUCCESS == TEEC_InvokeCommand(&session, STRING_UPPER_COMMAND, 
                                          &operation, &return_origin)) {
        printf("[API INFO:]  STRING_UPPER_COMMAND :%s \n", test_string);
        fflush(stdout);
    }
    
    TEEC_CloseSession(&session);

    TEEC_FinalizeContext(&context);
}
