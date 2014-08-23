#ifndef _CORE_TEE_TASK_H_
#define _CORE_TEE_TASK_H_

#include "tee_internal_api.h"
#include "tee_mem_mng.h"
#include "tee_list.h"
#include "cpu.h"

// 任务名的默认长度
#define TASK_NAME_LEN       (64)

// 任务所带参数的数量
#define TASK_PARAMS_NUM     (4)

// 任务堆栈大小(1 page)
#define TASK_STACK_SIZE      (1 << TEE_PAGE_SHIFT)

#define TASK_ID_START       (0x1000)


enum TASK_STATUS {
    TASK_READY,
    TASK_RUNNING,
    TASK_SUSPUD
};


// 任务数据 ,具有TASK_PARAMS_NUM个参数以及其它数据
typedef struct _TASK_DATA {
    void *params[TASK_PARAMS_NUM];
    void *other_data;
}TASK_DATA;

// 任务控制块 Task Control Block
typedef struct _TCB {
    // 任务上下文
    CPU_CORE_REGS core_regs;
    // 任务标识ID
    int32_t task_id;
    // 任务名
    char task_name[TASK_NAME_LEN + 1];
    // 任务入口地址
    uint32_t task_entry;
    // 任务数据
    TASK_DATA *task_data;
    // 任务堆栈大小
    uint32_t task_stack_size;
    // 任务堆栈栈顶地址
    uint32_t task_stack_top;
    // 任务状态
    enum TASK_STATUS task_status;
    // 所有任务列表
    LIST list_task;
    // 就绪任务列表
    LIST list_ready;
    // 挂起任务列表
    LIST list_suspud;
}TASK,TCB;

// 说明：创建任务函数，返回任务ID
// 返回：失败返回-1
int TaskCreate(TASK_DATA *task_data, uint32_t task_entry, char *task_name);

#endif