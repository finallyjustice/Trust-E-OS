#include "tee_main_task.h"

void TEE_StartMain(void)
{
    static bool flag = FALSE;
    int32_t task_id = -1;
    if (flag) {
        TEE_Printf("Only one main loop could be created!\n");
    }
    task_id = TaskCreate(NULL, (uint32_t)TEE_MainTask, "Main Task");
    if (task_id == -1) {
        TEE_Printf("Main loop created failed!\n");
        return ;
    }

    TA_CreateEntryPoint();

    // 启动 主任务
    TaskStart(task_id);
}

void TEE_MainTask(void)
{
    //uint32_t i;
    TEE_Result ret = 10000;
    uint32_t loop_cnt = 0;
    SESSION_CONTEXT *session_context;
    TZ_CMD_DATA *ns_tz_cmd_data = GetNonsecureCommand();

    LaunchNonSecure();

    while (1) {
        //TEE_Delay(1);
        TEE_Printf("Here is Main Task %d\n", ++loop_cnt);
        // 获取nonsecure的命令地址
        ns_tz_cmd_data = (TZ_CMD_DATA *)GetNonsecureCommand();
        // 将该命令映射到T-OS中
        ns_tz_cmd_data = (TZ_CMD_DATA *)map_from_ns((va_t)ns_tz_cmd_data);
        switch (ns_tz_cmd_data->operation_type) {
            case TEE_OPERATION_OPEN_SESSION:
                //TEE_Printf("Client requested to open session\n");
                session_context = &ns_tz_cmd_data->session_context;
                ret = TA_OpenSessionEntryPoint(ns_tz_cmd_data->param_type,
                                               ns_tz_cmd_data->params,
                                               (void **)&session_context);
                break;
            case TEE_OPERATION_CLOSE_SESSION:
                //TEE_Printf("Client requested to close session\n");
                TA_CloseSessionEntryPoint(&ns_tz_cmd_data->session_context);
                ret = TEE_SUCCESS;
                break;
            case TEE_OPERATION_INVOKE_COMMAND:
                TEE_Printf("Client requested to invoke command\n");
                //TEE_Printf("invoke command was not implemented!\n");
                ret = TA_InvokeCommandEntryPoint(&ns_tz_cmd_data->session_context,
                                                ns_tz_cmd_data->command_id,
                                                ns_tz_cmd_data->param_type,
                                                ns_tz_cmd_data->params);
                if (ret == TEE_SUCCESS) {
                    ret = GetTaskReturnValue();
                }
                break;
            default:
                TEE_Printf("Not a valid operation!\n");
                ret = TEE_ERROR_NOT_SUPPORTED;
                break;
        }
        TZApiReturn(ret);
        
    }
}