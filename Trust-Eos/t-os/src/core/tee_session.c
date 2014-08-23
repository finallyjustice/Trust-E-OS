#include "tee_session.h"
#include "tee_internal_api.h"
#include "tee_core_api.h"

// 取得合法的会话ID
int32_t GetValidSessionID(void)
{
    static int32_t session_id = SESSION_ID_START;
    return session_id++;
}