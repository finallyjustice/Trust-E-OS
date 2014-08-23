#ifndef _CORE_TEE_TA_INTERFACE_H_
#define _CORE_TEE_TA_INTERFACE_H_
#include "tee_internal_api.h"
#include "tee_task.h"

typedef struct _TASK_IDENTITY {
    TEE_UUID *uuid;
    void *task_addr;
    LIST list;
    char name[TASK_NAME_LEN];
}TASK_IDENTITY;

typedef struct _SERVICE_DATA {
    TEE_UUID uuid;
    void *task_entry_addr;
    char name[TASK_NAME_LEN];
}SERVICE_DATA;


// 4.3 TA Interface
#define TA_EXPORT


// 描述：This is the Trusted Application constructor. 
//      It is called once and only once in the life - time of the  Trusted  Application  instance. 
//      If this function fails, the instance is not created.
//      TA构造器。在TA实例的“声明”中该函数只能被调用一次。
//      如果该函数执行失败，则TA实例创建失败。
//      为了注册实例数据，够函数在实现过程中可以使用全局变量或者函数TEE_SetInstanceData
TEE_Result TA_EXPORT TA_CreateEntryPoint(void);


// 描述：This is the Trusted Application destructor. 
//      The Trusted Core Framework calls this function just before the Trusted Application instance is terminated.
//      The  Framework MUST guarantee that no sessions are open when this function is called.
//      When TA_DestroyEntryPoint  returns, the  Framework MUST collect all resources claimed by the Trusted Application instance.
//      TA析构函数.
//      系统在TA实例终止之前调用该函数，系统必须保证该函数调用的时候没有已经打开的会话。
//      当该函数返回时，系统必须回收TA实例中使用的所有资源。
void TA_EXPORT TA_DestroyEntryPoint(void);

// 描述：This function is called whenever a client attempts to connect to the Trusted Application instance to open a new session.
//      If this function returns an error, the connection is rejected and no new session is opened. 
//      In this function, the Trusted Application can attach an opaque void*  context to the session. 
//      This context is recalled in  all subsequent TA calls within the session. 
//      当客户端尝试连接TA实例的时候，该函数用于调用来打开一个新的会话。
//      如果该函数返回错误，连接被拒绝并且没有新会话打开。
//      在函数执行过程中，TA可以拥有一个void*上下文指向会话。
//      该上下文会被召回使用在接下来的使用该会话执行的TA调用中。
TEE_Result TA_EXPORT TA_OpenSessionEntryPoint(
                uint32_t    paramTypes,
        inout   TEE_Param   params[4],
        out ctx void**      sessionContext);

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
        ctx     void*       sessionContext);

// 描述：This function is called whenever a client invokes a Trusted Application command. 
//      The Framework gives back the session context reference to the Trusted Application in this function call.
//      在客户端调用TA命令的时候该函数被调用。
//      该函数在调用的过程中系统会把会话上下文的引用传回给TA。
TEE_Result TA_EXPORT TA_InvokeCommandEntryPoint(
        ctx     void*       sessionContext,
                uint32_t    commandID,
                uint32_t    paramTypes,
        inout   TEE_Param   params[4]);

void SetCurrentParams(TEE_Param params[4], uint32_t param_type, int32_t command);

void GetCurrentParams(TEE_Param **params, uint32_t *param_type, int32_t *command);

#endif // tee_ta_interface.h