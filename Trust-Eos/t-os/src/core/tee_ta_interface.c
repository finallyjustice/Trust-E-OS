#include "tee_internal_api.h"
#include "tee_core_api.h"
#include "tee_list.h"
#include "tee_session.h"
#include "tee_mmf.h"
#include "tee_debug.h"
#include "tee_ta_interface.h"
#include "communication/services.h"
#include "tee_string.h"

#include "tee_string_task.h"
#include "tee_test_robust_task.h"

static SERVICE_DATA service_data[] = {
        // string task
        {
            TASK_STRING_UUID,       // STRING TASK 的UUID
            Task_String,            // String Task 的入口地址
            "String Task",          // String Task 的名字
        },
        // test robust task
        {
            TASK_TEST_ROBUST_UUID,  // ROBUST TASK 的UUID
            Task_TestRobust,        // ROBUST TASK 的入口地址
            "Test Robust Task",     // ROBUST TASK 的名字
        }
    };




static LIST list_opened_sessions;
static LIST list_task_identities;

static TEE_Param *current_params = NULL;
static uint32_t current_param_type = 0;
static int32_t current_command = -1;


TEE_Result TA_EXPORT TA_CreateEntryPoint(void)
{
    int i=0;
    int service_name = 0;
    TASK_IDENTITY *task_identity = NULL;
    ListInit(&list_opened_sessions);
    ListInit(&list_task_identities);

    service_name  = sizeof(service_data) / sizeof(SERVICE_DATA);
    TEE_Printf("tasks numbuer:%d\n", service_name);
    for (i=0; i<service_name; i++) {
        task_identity = TEE_Malloc(sizeof(TASK_IDENTITY), 0);
        task_identity->uuid = &service_data[i].uuid;
        task_identity->task_addr = service_data[i].task_entry_addr;
        TEE_MemMove(task_identity->name, service_data[i].name, TASK_NAME_LEN);
        ListInit(&task_identity->list);
        ListAddTail(&list_task_identities, &task_identity->list);
    }


    return TEE_SUCCESS;
}

void TA_EXPORT TA_DestroyEntryPoint(void)
{
    if (!ListEmpty(&list_opened_sessions)) {
        TEE_Printf("Warning: Not all sessions are closed!\n");
    }
}


TEE_Result TA_EXPORT TA_OpenSessionEntryPoint(
                uint32_t    paramTypes,
        inout   TEE_Param   params[4],
        out ctx void**      sessionContext)
{
    static int32_t session_id = SESSION_ID_START;
    SESSION *session = NULL;
    SESSION_CONTEXT *session_context;
    TEE_UUID *dest_uuid, *client_uuid;
    LIST *p, *head;
    TASK_IDENTITY *task_identity;

    // 检查参数
    if (paramTypes == TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                                    TEE_PARAM_TYPE_MEMREF_INPUT,
                                    TEE_PARAM_TYPE_VALUE_INPUT,
                                    TEE_PARAM_TYPE_NONE)) {
        
        dest_uuid = (TEE_UUID *)map_from_ns((va_t)params[0].memref.buffer);
        client_uuid = (TEE_UUID *)map_from_ns((va_t)params[1].memref.buffer);

        // look for destination TASK_IDENTITY
        head = &list_task_identities;
        p = head;
        while (p->next != head) {
            p = p->next;
            task_identity = LIST_ENTRY(p, TASK_IDENTITY, list);
            if (TEE_MemCompare(task_identity->uuid, dest_uuid, sizeof(TEE_UUID)) == 0) {
                // already found
                session = (SESSION *)TEE_Malloc(sizeof(SESSION), 0);
                session->session_id = session_id++;
                session->task_id = TaskCreate(NULL, (uint32_t)task_identity->task_addr,
                                                task_identity->name);
                session->client_identity.login = params[2].value.a;
                session->client_identity.uuid = *client_uuid;
                ListInit(&session->list_all);
                ListAddTail(&list_opened_sessions, &session->list_all);
                session_context = *sessionContext;
                TEE_MemMove(&session_context->client_uuid, client_uuid, sizeof(SESSION_CONTEXT));
                session_context->session_id = session->session_id;
                TEE_Printf("A session was opened, id = 0x%x\n", session->session_id);
                return TEE_SUCCESS;
            }
        }
        return TEE_ERROR_NOT_SUPPORTED;
    }
    else {
        return TEE_ERROR_BAD_PARAMETERS;
    }
}

// 描述：This function is called when the client closes a session and disconnects from the Trusted Application instance. 
//      The Implementation guarantees that there are no active commands in the session being closed.
//      The session context reference is given back to the Trusted Application by the Framework. 
//      It is the responsibility of the Trusted Application to deallocate the session context if memory has been allocated for it.  
//      该函数在客户端关闭会话以及从TA实例断开连接的时候使用。
//      在会话被关闭的时候必须保证没有active命令。
//      系统会把上下文的返回给TA。
//      如果会话在执行过程中分配了内存，TA具有职责将其回收。
//      (一个TA具有多个会话）
void TA_EXPORT TA_CloseSessionEntryPoint(
        ctx     void*       sessionContext)
{
    // 检查sessionContext是否合法
    SESSION_CONTEXT *session_context = NULL; //sessionContext;
    SESSION *session = NULL;
    LIST *p, *head;
    p = head = &list_opened_sessions;
    session_context = (SESSION_CONTEXT *)sessionContext;
    while (p->next != head) {
        p = p->next;
        session = LIST_ENTRY(p, SESSION, list_all);
        if (TEE_MemCompare(&session->client_identity.uuid, 
                        &session_context->client_uuid, sizeof(TEE_UUID)) == 0 
            && session->session_id == session_context->session_id) {
            // 找到了指定的会话
            TaskDelReadyByID(session->task_id);
            TaskDelSuspudByID(session->task_id);
            ListDel(&list_opened_sessions, &session->list_all);
            TEE_Printf("A session was closed, id = 0x%x\n", session->session_id);
            TEE_Free(session);
            return;;
        }
    }
    // 未找到指定的会话
    TEE_Printf("T-OS:Close Session failed! Not found the specific session\n");
}

// 描述：This function is called whenever a client invokes a Trusted Application command. 
//      The Framework gives back the session context reference to the Trusted Application in this function call.
//      在客户端调用TA命令的时候该函数被调用。
//      该函数在调用的过程中系统会把会话上下文的引用传回给TA。
TEE_Result TA_EXPORT TA_InvokeCommandEntryPoint(
        ctx     void*       sessionContext,
                uint32_t    commandID,
                uint32_t    paramTypes,
        inout   TEE_Param   params[4])
{
    // look for session
    LIST *p, *head;
    head = &list_opened_sessions;
    p = head;
    SESSION *session = NULL;
    SESSION_CONTEXT *session_context = (SESSION_CONTEXT *)sessionContext;

    while (p->next != head) {
        p = p->next;
        session = LIST_ENTRY(p, SESSION, list_all);
        if (TEE_MemCompare(&session->client_identity.uuid, 
                           &session_context->client_uuid,
                           sizeof(TEE_UUID)) == 0 
            && session->session_id == session_context->session_id) {
            // session was found
            SetCurrentParams(params, paramTypes, commandID);
            TaskStart(session->task_id);
            return TEE_SUCCESS;
        }
    }

    return TEE_ERROR_BAD_PARAMETERS;
}


void SetCurrentParams(TEE_Param params[4], uint32_t param_type, int32_t command)
{
    current_params = params;
    current_param_type = param_type;
    current_command = command;
}

void GetCurrentParams(TEE_Param **params, uint32_t *param_type, int32_t *command)
{
    *params = current_params;
    *param_type = current_param_type;
    *command = current_command;
}
