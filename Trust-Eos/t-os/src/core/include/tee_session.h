#ifndef _CORE_TEE_SESSION_H_
#define _CORE_TEE_SESSION_H_

#include "tee_task.h"
#include "tee_internal_api.h"
#include "tee_list.h"
#include "tee_core_api.h"

#define SESSION_ID_START        (0x1000)

typedef struct _SESSION {
    // 会话ID
    int32_t session_id;
    // 会话对应的任务ID
    int32_t task_id;
    // 客户端标识
    TEE_Identity client_identity;
    // 所有会话链表
    LIST list_all;
    // 当前任务所对应会话的链表
    LIST list_cur_task;
}SESSION;

// 取得合法的会话ID
int32_t GetValidSessionID(void);

#endif // tee_session.h