#include "client_include.h"

char g_stack_sys[4096];
char g_stack_fiq[4096];
char g_stack_irq[4096]; 
char g_stack_svc[4096];
char g_stack_abt[4096];
char g_stack_undef[4096];




TEE_UUID task_string_uuid = {
                            0xD1059173, 
                            0x97A7, 
                            0xED77, 
                            {
                             0x61, 0x9D, 0xAB,
                             0x32, 0x6D, 0xBA,
                             0x89, 0x4B
                            }
                            };  

void Delay(mill_seconds)
{
    volatile uint32_t i,j,k;
    for (i=0; i!= mill_seconds; i++) {
        for (j=0; j<0xfff; j++) {
            for (k=0; k<0xfff; k++) {
            }
        }
    }
}

void PrintUUID(TEE_UUID *uuid)
{
    Client_Printf("%x-%x-%x-%x%x%x%x%x%x%x%x\n",
                    uuid->timeLow,
                    uuid->timeMid,
                    uuid->timeHiAndVersion,
                    uuid->clockSeqAndNode[0],
                    uuid->clockSeqAndNode[1],
                    uuid->clockSeqAndNode[2],
                    uuid->clockSeqAndNode[3],
                    uuid->clockSeqAndNode[4],
                    uuid->clockSeqAndNode[5],
                    uuid->clockSeqAndNode[6],
                    uuid->clockSeqAndNode[7]);
}

// user layer
int main()
{
    int ret = -1;
    int loop_cnt = 0;
    TEE_Param params[4];
    SESSION_CONTEXT session_context;
    char *str = NULL;

    MallocInit();
    pl011_uart_init(0);

    str = (char*)UserMalloc(100);
    

    uint32_t param_type;
    
    while(1) {
        // app1

        Delay(1);
        Client_Printf("client loop %d\n", ++loop_cnt);
        params[0].memref.buffer = UserMalloc(sizeof(TEE_UUID));
        params[0].memref.size = sizeof(TEE_UUID);
        MemMove(params[0].memref.buffer, &task_string_uuid, sizeof(TEE_UUID));
        param_type = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
                                    TEE_PARAM_TYPE_NONE,
                                    TEE_PARAM_TYPE_NONE,
                                    TEE_PARAM_TYPE_NONE);
        ret = OpenSession(params, &session_context, param_type);
        if (ret != TEE_SUCCESS) {
            Client_Printf("Open Session failed!\n");
            while(1);

        }
        Client_Printf("Open Session successfully!\n");
        Client_Printf(".....Client UUID:\n");
        PrintUUID(&session_context.client_uuid);
        Client_Printf(".....session_id:0x%x\n",session_context.session_id );

        // call lower command
        Client_MemFill(params, 0, sizeof(TEE_Param) * 4);
        Client_StrCopy(str, "HELLO T-OS");
        Client_Printf("client string:%s\n",str);
        params[0].memref.buffer = str;
        params[0].memref.size = Client_StrLen(str) + 1;
        param_type = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT,
                                    TEE_PARAM_TYPE_NONE,
                                    TEE_PARAM_TYPE_NONE,
                                    TEE_PARAM_TYPE_NONE);
        ret = InvokeSessionCommand(&session_context, params, 
                                    param_type, STRING_LOWER_COMMAND);
        if (ret != TEE_SUCCESS) {
            Client_Printf("call command failed!\n");
        }
        Client_Printf("client lower string:%s\n",str);

        ret = CloseSession(&session_context);
        if (ret == TEE_SUCCESS) {
            Client_Printf("Close session successfully\n");
        }
        else {
            Client_Printf("Close session failed!\n");
        }

        Client_StrCopy(str, "what's your name?");
        Client_Printf("client string:%s\n",str);
        params[0].memref.buffer = str;
        params[0].memref.size = Client_StrLen(str) + 1;
        param_type = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT,
                                    TEE_PARAM_TYPE_NONE,
                                    TEE_PARAM_TYPE_NONE,
                                    TEE_PARAM_TYPE_NONE);
        ret = InvokeSessionCommand(&session_context, params, 
                                    param_type, STRING_UPPER_COMMAND);
         if (ret != TEE_SUCCESS) {
            Client_Printf("call command failed!\n");
        }
        Client_Printf("client lower string:%s\n",str);
        continue;
    }
    return ret;
}