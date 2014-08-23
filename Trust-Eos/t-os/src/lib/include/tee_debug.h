/*
 * TEE 串口调试信息输出
 */
#include "tee_internal_api.h"
#include <stdarg.h>

// 格式化输出字符串
uint32_t TEE_Printf(const char *fmt, ...);

uint32_t sw_sprintf(char *print_buffer, const char *fmt, ...);
// 格式化字符串
int sw_vsprintf(char *buf, const char *fmt, va_list args);

// 调试输出单个字符
void debug_putc(char c);

// 调试输出字符串
void debug_puts(char *str);

// 调试初始化
void debug_init(void);