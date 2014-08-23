#ifndef _CORE_TEE_SCHEDULER_H_
#define _CORE_TEE_SCHEDULER_H_

#include "tee_task.h"

// 说明：任务调度
void TaskSchedule();

// 说明：TaskSwitch作为一个由_svc_task_schedule调用的子函数
//      该函数被调用的时候,g_temp_regs已经存放有当前任务的上下文，
//      该函数保存当前任务上下文，然后将下一任务的上下文暂存入至
//      g_temp_regs中，回到_svc_task_schedule中恢复下一任务的上下文
void TaskSwitch();


// 说明：任务管理模块初始化
void TaskInit(void);

// 说明：任务启动
void TaskStart(int task_id);

// 说明：获取下一个就就绪任务
// 返回：如果没有，返回NULL
//TASK* TaskNextReady(void);

// 说明：从就绪列表中删除
void TaskDelReady(TASK *task);

// 说明：加入就绪列表
void TaskAddReady(TASK *task);

// 说明：从就绪列表中删除
void TaskDelReadyByID(int32_t task_id);

// 说明：从挂起列表中删除
void TaskDelSuspudByID(int32_t task_id);

// 说明：从挂起列表中删除
void TaskDelSuspud(TASK *task);

// 说明：加入到挂起列表
void TaskAddSuspud(TASK *task);

// 说明：任务处理完毕，任务返回
void TaskHandleReturn(int32_t ret);

void SetTaskReturnValue(int32_t ret);

int32_t GetTaskReturnValue( void );

// for testing
int32_t TaskNumSuspud();

// for testing
int32_t TaskNumReady();

#endif