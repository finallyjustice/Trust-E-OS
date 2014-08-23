#include "tee_core_api.h"
#include "page_table.h"
#include "cpu_asm.h"

/* 页表空间 */
unsigned int page_table[PAGE_TABLE_ENTRIES] __attribute__ ((section (".data")))
                                        __attribute__ ((aligned (4)));

/* 用户/系统模式栈空间 */
uint32_t user_stack[STACK_SIZE] __attribute__ ((aligned (4)));

/* FIQ模式栈空间 */
uint32_t fiq_stack[STACK_SIZE] __attribute__ ((aligned (4)));

/* Supervisor(监视器)模式栈空间 */
uint32_t supervisor_stack[STACK_SIZE] __attribute__ ((aligned (4)));

/* Abort模式栈空间 */
uint32_t abort_stack[STACK_SIZE] __attribute__ ((aligned (4)));

/* IRQ模式栈空间 */
uint32_t irq_stack[STACK_SIZE] __attribute__ ((aligned (4)));

/* Undefined模式栈空间 */
uint32_t undefined_stack[STACK_SIZE] __attribute__ ((aligned (4)));

/* Monitor模式栈空间 */
uint32_t monitor_stack[STACK_SIZE] __attribute__ ((aligned (4)));
