#include "s5pv210_regs_base.h"
#include "s5pv210_uart.h"


void uart_init(void)
{
    volatile S5PV210_UART *const uart = (S5PV210_UART *)UART_BASE;
    uart->UFCON = 0;   // FIFO disabled
    uart->UMCON = 0;   // RTS AFC disabled
    uart->ULCON = 0x3; // 8-N-1
    uart->UCON = 0xe45;  // polling mode - external clock: DIV_VAL = (53213366¡à(115200¡ß16))¡¾1=27.87
    uart->UBRDIV = 0x22; // integer part of div_val = 27
    uart->UDIVSLOT = 0x1FFF; // defined(CONFIG_UART_66)
}
void uart_putc(char c)
{
    volatile S5PV210_UART *const uart = (S5PV210_UART *)UART_BASE;
    while (!(uart->UTRSTAT & 0x2));
    uart->UTXH = c;
}

void uart_puts(char *s)
{
    int index = 0;

    while (s[index] != '\0')
    {
        uart_putc(s[index]);
        index++;
    }
}