#include "tee_list.h"
#include "tee_task.h"
#include "tee_scheduler.h"
#include "cpu_asm.h"
#include "tee_debug.h"
#include "tee_mmf.h"

// 用于临时存储当前CPU上下文
extern CPU_CORE_REGS  g_temp_regs;

// 当前任务
static TASK *current_task;

// 任务列表
static LIST list_all_tasks;
// 就绪任务列表
static LIST list_ready_tasks;
// 挂起任务列表
static LIST list_suspud_tasks;

static int32_t task_return_value;

// 说明：任务管理模块初始化
void TaskInit(void)
{
    static uint32_t flag = FALSE;
    if (flag) {
        TEE_Printf("Task Module has been inited sometime before!\n");
        return;
    }
    flag = TRUE;
    current_task = NULL;
    ListInit(&list_all_tasks);
    ListInit(&list_ready_tasks);
    ListInit(&list_suspud_tasks);
}

// 说明：设置当前任务
static inline void SetCurrentTask(TASK *task)
{
    current_task = task;
}

// 说明：获取当前任务TCB*
static inline TASK* TaskCurrent()
{
    return current_task;
}

// 说明：获取下一个就就绪任务
// 返回：如果没有，返回NULL
static TASK* TaskNextReady(void)
{
    // 如果存在下一个就绪任务
    if (!ListEmpty(&list_ready_tasks) && 
        list_ready_tasks.next != &current_task->list_ready) {
        return LIST_ENTRY(list_ready_tasks.next, TASK, list_ready);
    }
    return NULL;
}

// 说明：从就绪列表中删除
void TaskDelReady(TASK *task)
{
    ListDel(&list_ready_tasks, &task->list_ready);
}

void TaskDelReadyByID(int32_t task_id)
{
    LIST *p, *head;
    TASK *task;
    p = head = &list_ready_tasks;
    while (p->next != head) {
        p = p->next;
        task = LIST_ENTRY(p, TASK, list_ready);
        if (task->task_id == task_id) {
            // found
            TaskDelReady(task);
            break;
        }
    }
}

void TaskDelSuspudByID(int32_t task_id)
{
    LIST *p, *head;
    TASK *task; 
    p = head = &list_suspud_tasks;
    while (p->next != head) {
        p = p->next;
        task = LIST_ENTRY(p, TASK, list_suspud);
        if (task->task_id == task_id) {
            // found
            TaskDelReady(task);
            break;
        }
    }
}


// 说明：加入就绪列表
void TaskAddReady(TASK *task)
{
    ListAddTail(&list_ready_tasks, &task->list_ready);
}

// 说明：从挂起列表中删除
void TaskDelSuspud(TASK *task)
{
    ListDel(&list_suspud_tasks, &task->list_suspud);
}

// 说明：加入到挂起列表
void TaskAddSuspud(TASK *task)
{
    ListAdd(&list_suspud_tasks, &task->list_suspud); 
}


void TaskSchedule()
{

    // push r0
    asm volatile ("stmfd    sp!, {r0}");
    register uint32_t r0 asm("r0") = SVC_SCHEDULE_FLAG; 
    r0 = r0;
    asm volatile ("svc #0");
    // pop r0
    asm volatile ("ldmfd    sp!, {r0}");
}

void TaskSwitch()
{
    TASK *current,*next;
    current = TaskCurrent();
    next = TaskNextReady();
    if (next) { 
        if (current) {
            // 如果当前任务是主任务（即第一个任务main），
            // 则将其继续加入就绪列表，不要挂起
            if (current->task_id == TASK_ID_START) {
                current->task_status = TASK_READY;
                TaskAddReady(current);
            }
            else {
                current->task_status = TASK_SUSPUD; 
                TaskAddSuspud(current);
                TEE_Printf("in task switch...\n"); 
            }
            TEE_MemMove(&current->core_regs, &g_temp_regs, sizeof(CPU_CORE_REGS));
        }
        next->task_status = TASK_RUNNING;
        SetCurrentTask(next);
        TaskDelReady(next);
        TEE_MemMove(&g_temp_regs, &next->core_regs, sizeof(CPU_CORE_REGS));
    }
}

// 说明：任务启动
void TaskStart(int task_id)
{
    TASK *task = NULL;
    LIST *head = &list_suspud_tasks;
    LIST *p = head;
    while (p->next != head) {
        p = p->next;
        task = LIST_ENTRY(p, TASK, list_suspud);
        if (task->task_id == task_id) {
            TaskDelSuspud(task);
            TaskAddReady(task);
            TaskSchedule();
            return;
        } 
    }
}


// for testing
int32_t TaskNumSuspud()
{
    TASK *task;
    int32_t num = 0;
    LIST *head, *p;
    head = &list_suspud_tasks;
    p = head;
    while (p->next != head) {
        num++;
        p = p->next;
        task = LIST_ENTRY(p, TASK, list_suspud);
        TEE_Printf("%d : tid:0x%x  name:%s\n",
                    num, task->task_id, task->task_name);
        //p = p->next;
    }
    return num;
}

// for testing
int32_t TaskNumReady()
{
    TASK *task;
    int32_t num = 0;
    LIST *head, *p;
    head = &list_ready_tasks;
    p = head;

    while (p->next != head) {
        num++;
        p = p->next;
        task = LIST_ENTRY(p, TASK, list_ready);
        TEE_Printf("%d : tid:0x%x  name:%s\n",
                    num, task->task_id, task->task_name);
    }
    return  num;
}

// 说明：任务处理完毕，任务返回
void TaskHandleReturn(int32_t ret)
{
    ret = ret;
    TaskSchedule();
}

void SetTaskReturnValue(int32_t ret)
{
    task_return_value = ret;
}

int32_t GetTaskReturnValue( void )
{
    return task_return_value;
}

