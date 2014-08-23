#include "client_api/tz_api.h"
#include "unistd.h"

static TEEC_UUID string_task_uuid = TASK_STRING_UUID;
static TEEC_UUID test_robust_task_uuid = TASK_TEST_ROBUST_UUID;

const char types_str[][40] = {
    "TEEC_NONE",     //0x0
    "TEEC_VALUE_INPUT",     //1
    "TEEC_VALUE_OUTPUT",     //2
    "TEEC_VALUE_INOUT",     //3
    "",     //4
    "TEEC_MEMREF_TEMP_INPUT",     //5
    "TEEC_MEMREF_TEMP_OUTPUT",     //6
    "TEEC_MEMREF_TEMP_INOUT",     //7
    "",     //8
    "",     //9
    "",     //a
    "",     //b
    "TEEC_MEMREF_WHOLE",     //c
    "TEEC_MEMREF_PARTIAL_INPUT",     //d
    "TEEC_MEMREF_PARTIAL_OUTPUT",     //e
    "TEEC_MEMREF_PARTIAL_INOUT"      //f
};

// 测试通信的健壮性
// 4个参数，每个参数有11种选择，一共有 11 * 11 * 11 * 11 种测试用例
void TestCommunicationRobust(TEEC_Session *session)
{
    const char src_str[] = "I am only a test example, such as Li Tao";
    int all_types[11] = {TEEC_NONE,TEEC_VALUE_INPUT,TEEC_VALUE_OUTPUT,TEEC_VALUE_INOUT,
                    TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_MEMREF_TEMP_INOUT,
                    TEEC_MEMREF_WHOLE, TEEC_MEMREF_PARTIAL_INPUT, TEEC_MEMREF_PARTIAL_OUTPUT,
                    TEEC_MEMREF_PARTIAL_INOUT};
    char shared_buffer[4][100];
    int types[4];
    int i,j,k,g,index;
    int t;
    int count = 0;
    TEEC_SharedMemory shared_memory[4];
    TEEC_Operation operation;

    for (i=0; i<11; i++) {
    // 参数0循环
        types[0] = all_types[i];
        for (j=0; j<11; j++) {
        // 参数1循环
            types[1] = all_types[j];
            for (k=0; k<11; k++) {
            // 参数2循环
                types[2] = all_types[k];
                for (g=0; g<11; g++) {
                // 参数3循环
                    types[3] = all_types[g];
                    operation.paramTypes = TEEC_PARAM_TYPES(types[0], types[1], types[2], types[3]);
                    count++;
                    printf("[USER INFO:] test times:%d\n", count); 
                    printf("[USER INFO:]:---input---\n");
                    fflush(stdout);
                    for (index=0; index<4; index++) {
                        t = TEEC_PARAM_TYPE_GET(operation.paramTypes,index);
                        if (t == TEEC_VALUE_INPUT || t == TEEC_VALUE_OUTPUT || t == TEEC_VALUE_INOUT) {
                            operation.params[index].value.a = 1;
                            operation.params[index].value.b = 2;
                            printf("[USER INFO:] %s = 0x%x 0x%x\n",types_str[t], operation.params[index].value.a, operation.params[index].value.b);
                            fflush(stdout);
                        }
                        else if (t == TEEC_MEMREF_TEMP_INPUT || t == TEEC_MEMREF_TEMP_OUTPUT || t == TEEC_MEMREF_TEMP_INOUT) {
                            operation.params[index].tmpref.size = strlen(src_str) + 1;
                            memcpy(shared_buffer[index], src_str, operation.params[index].tmpref.size);
                            operation.params[index].tmpref.buffer = shared_buffer[index];
                            printf("[USER INFO:] %s = %s 0x%x\n",types_str[t], (char *)operation.params[index].tmpref.buffer, operation.params[index].tmpref.size);
                            fflush(stdout);
                        }
                        else if (t == TEEC_MEMREF_WHOLE || t == TEEC_MEMREF_PARTIAL_INPUT || t == TEEC_MEMREF_PARTIAL_OUTPUT || t == TEEC_MEMREF_PARTIAL_INOUT) {
                            shared_memory[index].buffer = shared_buffer[index];
                            shared_memory[index].size = 100;
                            shared_memory[index].flags = 0;
                            memcpy(shared_memory[index].buffer, src_str, strlen(src_str) + 1);
                            operation.params[index].memref.parent = &shared_memory[index];
                            operation.params[index].memref.size = strlen(src_str) + 1;
                            operation.params[index].memref.offset = 0;
                            printf("[USER INFO:] %s = %s 0x%x\n",types_str[t], (char *)shared_memory[index].buffer, operation.params[index].memref.size);
                            fflush(stdout);
                        }
                        else {
                            printf("[USER INFO:] %s\n",types_str[t]);
                            fflush(stdout);
                        } 
                    }
                    TEEC_InvokeCommand(session, TEST_ROBUST_COMMUNICATION_COMMAND, &operation, NULL); 
                    printf("[USER INFO:]:---output---\n"); 
                    for (index=0; index<4; index++) {
                        t = TEEC_PARAM_TYPE_GET(operation.paramTypes,index);
                        if (t == TEEC_VALUE_INPUT || t == TEEC_VALUE_OUTPUT || t == TEEC_VALUE_INOUT) {
                            printf("[USER INFO:] %s = 0x%x 0x%x\n",types_str[t], operation.params[index].value.a, operation.params[index].value.b);
                        }
                        else if (t == TEEC_MEMREF_TEMP_INPUT || t == TEEC_MEMREF_TEMP_OUTPUT || t == TEEC_MEMREF_TEMP_INOUT) {
                            printf("[USER INFO:] %s = %s 0x%x\n",types_str[t], (char *)operation.params[index].tmpref.buffer, operation.params[index].tmpref.size);
                        }
                        else if (t == TEEC_MEMREF_WHOLE || t == TEEC_MEMREF_PARTIAL_INPUT || t == TEEC_MEMREF_PARTIAL_OUTPUT || t == TEEC_MEMREF_PARTIAL_INOUT) {
                            printf("[USER INFO:] %s = %s 0x%x\n",types_str[t], (char *)shared_memory[index].buffer, operation.params[index].memref.size);
                        }
                        else {
                            printf("[USER INFO:] %s\n",types_str[t]);
                        }
                    }
                    fflush(stdout);
                    //usleep(50);
                }
            }
        }
    }

}

int main()
{   
    // 测试OpenSessioin
    char name[] ="/dev/trustzone_driver";
    char test_string[] = "hello WORLD";
    TEEC_Context context;
    TEEC_Session session;
    TEEC_Operation operation;
    uint32_t return_origin;

    //operation.params[0].tmpref.buffer = test_string;
    //operation.params[0].tmpref.size = strlen(test_string) + 1;
    //operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT,
    //                                        TEEC_NONE, TEEC_NONE, TEEC_NONE);
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

    if (TEEC_SUCCESS != TEEC_OpenSession(&context, &session, &test_robust_task_uuid, 0, NULL, NULL, &return_origin)) {
        printf("[API INFO:] TEEC_OpenSession failed \n");
        return 0;
    }
    printf("[API INFO:] open session ok, session_id = 0x%x \n", session.session_context.session_id);
    fflush(stdout);

    // if (TEEC_SUCCESS == TEEC_InvokeCommand(&session, STRING_LOWER_COMMAND, 
    //                                     &operation, &return_origin)) {
    //     printf("[API INFO:]  STRING_LOWER_COMMAND :%s \n", test_string);
    //     fflush(stdout);
    // }
    

    // if (TEEC_SUCCESS == TEEC_InvokeCommand(&session, STRING_UPPER_COMMAND, 
    //                                       &operation, &return_origin)) {
    //     printf("[API INFO:]  STRING_UPPER_COMMAND :%s \n", test_string);
    //     fflush(stdout);
    // }
    
    TestCommunicationRobust(&session);
    TEEC_CloseSession(&session);

    TEEC_FinalizeContext(&context);
    return 0;
}