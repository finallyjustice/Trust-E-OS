#include "pl011_uart.h"


static inline unsigned int get_uart_base_addr(unsigned int id);

unsigned int read_uart(unsigned int reg_offs, unsigned int uartid)
{
    volatile unsigned int * reg_ptr = (unsigned int*)(get_uart_base_addr(uartid) | reg_offs);
    return *reg_ptr;
}

void write_uart(unsigned int reg_offs, unsigned int value, unsigned int uartid)
{
    volatile unsigned int * reg_ptr = (unsigned int*)(get_uart_base_addr(uartid) | reg_offs);
    *reg_ptr = value;
}

static inline unsigned int get_uart_base_addr(unsigned int id)
{
    switch(id) {
        case 0:
            return PL011_UART0_BASE;
        case 1:
            return PL011_UART1_BASE;
        case 2:
            return PL011_UART2_BASE;
        case 3:
            return PL011_UART3_BASE;
        default:
            break;
    }
    return -1;
}

void pl011_uart_putc(char c)
{
    /* Wait until there is space in the FIFO */
    while (read_uart(UART_PL01x_FR, 0) & UART_PL01x_FR_TXFF);

    /* Send the character */
    write_uart(UART_PL01x_DR, c, 0);
}

void pl011_uart_puts(char * c)
{
    int index = 0;

    while (c[index] != '\0')
    {
        pl011_uart_putc(c[index]);
        index++;
    }
}

void pl011_uart_init(unsigned int uart_id)
{
    int lcrh_reg;
    /* First, disable everything */
    write_uart(UART_PL011_CR, 0x0, uart_id);

    lcrh_reg = read_uart(UART_PL011_LCRH, uart_id);
    lcrh_reg &= ~UART_PL011_LCRH_FEN;
    write_uart(UART_PL011_LCRH, lcrh_reg, uart_id);

    write_uart(UART_PL011_IBRD, 13, uart_id);

    write_uart(UART_PL011_LCRH, (UART_PL011_LCRH_WLEN_8 | UART_PL011_LCRH_FEN), uart_id);

    /* Finally, enable the UART */
    write_uart(UART_PL011_CR, (UART_PL011_CR_UARTEN | UART_PL011_CR_TXE), uart_id);

    return;
}