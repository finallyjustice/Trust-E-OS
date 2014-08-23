#ifndef _BOARD_PL011_UART_H_
#define _BOARD_PL011_UART_H_

#include "eb_regs_base.h"

#define UART_PL01x_FR                   0x18     /*  Flag register (Read only). */
#define UART_PL01x_FR_TXFF              0x20
#define UART_PL01x_DR                   0x00     /*  Data read or written from the interface. */
#define UART_PL011_CR                   0x30
#define UART_PL011_LCRH                 0x2C
#define UART_PL011_LCRH_FEN             (1 << 4)
#define UART_PL011_LCRH                 0x2C
#define UART_PL011_IBRD                 0x24
#define UART_PL011_LCRH_WLEN_8          (3 << 5)
#define UART_PL011_CR_UARTEN            (1 << 0)
#define UART_PL011_CR_TXE               (1 << 8)

void pl011_uart_init(unsigned int uart_id);
void pl011_uart_puts(char * c);
void pl011_uart_putc(char c);

#endif