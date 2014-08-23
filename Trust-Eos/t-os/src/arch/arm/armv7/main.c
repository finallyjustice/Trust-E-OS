/*
 * T-OS的C程序主函数（入口函数）
 */
#include "tee_internal_api.h"
#include "tee_debug.h"
#include "tee_mem_mng.h"
#include "tee_bound_tag.h"
#include "tee_global.h"
#include "tee_mmf.h"
#include "tee_task.h"
#include "tee_scheduler.h"
#include "tee_timer.h"
#include "tee_main_task.h"

 extern uint32_t _TZ_CODE_END;

CPU_CORE_REGS  g_temp_regs;


int main()
{
    // 全局初始化函数，该函数必须在main开始时候调用
    global_init();

    TEE_Printf("Hello S5PV210\n");
    //while(TRUE);
    // 启动主函数
    //Main_CreateEntryPoint();
    TEE_StartMain();

    
    while(1);
    TEE_Printf("continue...\n");
    return 0;
}

