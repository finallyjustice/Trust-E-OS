#include "tee_debug.h"
#ifdef S5PV210
#include "s5pv210_uart.h"
#elif FVP_EB_Cortex_A8 
#include "pl011_uart.h"
#endif


// 调试输出单个字符
void debug_putc(char c)
{
#ifdef S5PV210
    uart_putc(c);
#elif FVP_EB_Cortex_A8
    pl011_uart_putc(c);
#else
    "error, no valid platform"
#endif
}

// 调试输出字符串
void debug_puts(char *str)
{
#ifdef S5PV210
    uart_puts(str);
#elif FVP_EB_Cortex_A8
    pl011_uart_puts(str);
#endif
}

void debug_init(void)
{ 
#ifdef S5PV210
    uart_init();
#elif FVP_EB_Cortex_A8
    pl011_uart_init(0);
#endif
}