#include "tee_internal_api.h"
#include "tee_task.h"
#include "tee_list.h"
#include "tee_mmf.h"
#include "tee_string.h"
#include "cpu_asm.h"
#include "tee_scheduler.h"

// 说明：创建任务函数，返回任务ID
// 返回：失败返回-1
int TaskCreate(TASK_DATA *task_data, uint32_t task_entry, char *task_name)
{
    static int32_t id = TASK_ID_START;
    uint32_t len = 0;
    uint32_t *stack_bottom = NULL;
    TASK *new_task = NULL;

    new_task = TEE_Malloc(sizeof(TASK), 0);
    if (!new_task) {
        return -1;
    }

    TEE_MemFill(new_task, 0, sizeof(TASK));

    new_task->task_id = id++;
    len = TEE_StrLen(task_name);
    TEE_MemMove(new_task->task_name, task_name, 
                len < TASK_NAME_LEN ? len : TASK_NAME_LEN);
    new_task->task_entry = task_entry;
    new_task->task_data = task_data;

    // 任务栈
    new_task->task_stack_size = (TASK_STACK_SIZE & 0x3) == 0 ?
                                (TASK_STACK_SIZE) :
                                ((TASK_STACK_SIZE & 0x3)+4);
    stack_bottom = TEE_Malloc(new_task->task_stack_size, 0);
    new_task->task_stack_top = (uint32_t)stack_bottom + 
                                new_task->task_stack_size;

    // 任务状态
    new_task->task_status = TASK_SUSPUD;

    // CPU上下文初始化
    new_task->core_regs.r0 = 0x00000000;
    new_task->core_regs.r1 = 0x01010101;
    new_task->core_regs.r2 = 0x02020202;
    new_task->core_regs.r3 = 0x03030303;
    new_task->core_regs.r4 = 0x04040404;
    new_task->core_regs.r5 = 0x05050505;
    new_task->core_regs.r6 = 0x06060606;
    new_task->core_regs.r7 = 0x07070707;
    new_task->core_regs.r8 = 0x08080808;
    new_task->core_regs.r9 = 0x09090909;
    new_task->core_regs.r10 = 0x10101010;
    new_task->core_regs.r11 = 0x11111111;
    new_task->core_regs.r12 = 0x12121212;
    new_task->core_regs.sp = new_task->task_stack_top;
    new_task->core_regs.lr = 0x14141414;
    new_task->core_regs.pc = task_entry;
    // system mode, disable IRQ, disable FIQ
    new_task->core_regs.spsr = MODE_SYS | I_BIT | F_BIT;

    // 任务链表
    ListInit(&new_task->list_task);
    ListInit(&new_task->list_ready);
    ListInit(&new_task->list_suspud);

    // 加入到挂起链表中
    //ListAddTail(&list_suspud_tasks, &new_task->list_suspud);
    TaskAddSuspud(new_task);
    return new_task->task_id;
}
