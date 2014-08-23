#define UART_SIZE             0x00010000

#define PL011_UART0_BASE         (0x10009000)
#define PL011_UART1_BASE         (0x1000A000)
#define PL011_UART2_BASE         (0x1000B000)
#define PL011_UART3_BASE         (0x1000C000)
/*
 *  PL011 definitions
 *
 */
#define UART_PL011_IBRD                 0x24
#define UART_PL011_FBRD                 0x28
#define UART_PL011_LCRH                 0x2C
#define UART_PL011_CR                   0x30
#define UART_PL011_IMSC                 0x38
#define UART_PL011_MIS                  0x40
#define UART_PL011_ICR                  0x44
#define UART_PL011_PERIPH_ID0           0xFE0

#define UART_PL011_LCRH_SPS             (1 << 7)
#define UART_PL011_LCRH_WLEN_8          (3 << 5)
#define UART_PL011_LCRH_WLEN_7          (2 << 5)
#define UART_PL011_LCRH_WLEN_6          (1 << 5)
#define UART_PL011_LCRH_WLEN_5          (0 << 5)
#define UART_PL011_LCRH_FEN             (1 << 4)
#define UART_PL011_LCRH_STP2            (1 << 3)
#define UART_PL011_LCRH_EPS             (1 << 2)
#define UART_PL011_LCRH_PEN             (1 << 1)
#define UART_PL011_LCRH_BRK             (1 << 0)

#define UART_PL011_CR_CTSEN             (1 << 15)
#define UART_PL011_CR_RTSEN             (1 << 14)
#define UART_PL011_CR_OUT2              (1 << 13)
#define UART_PL011_CR_OUT1              (1 << 12)
#define UART_PL011_CR_RTS               (1 << 11)
#define UART_PL011_CR_DTR               (1 << 10)
#define UART_PL011_CR_RXE               (1 << 9)
#define UART_PL011_CR_TXE               (1 << 8)
#define UART_PL011_CR_LPE               (1 << 7)
#define UART_PL011_CR_IIRLP             (1 << 2)
#define UART_PL011_CR_SIREN             (1 << 1)
#define UART_PL011_CR_UARTEN            (1 << 0)

#define UART_PL011_IMSC_OEIM            (1 << 10)
#define UART_PL011_IMSC_BEIM            (1 << 9)
#define UART_PL011_IMSC_PEIM            (1 << 8)
#define UART_PL011_IMSC_FEIM            (1 << 7)
#define UART_PL011_IMSC_RTIM            (1 << 6)
#define UART_PL011_IMSC_TXIM            (1 << 5)
#define UART_PL011_IMSC_RXIM            (1 << 4)
#define UART_PL011_IMSC_DSRMIM          (1 << 3)
#define UART_PL011_IMSC_DCDMIM          (1 << 2)
#define UART_PL011_IMSC_CTSMIM          (1 << 1)
#define UART_PL011_IMSC_RIMIM           (1 << 0)

#define UART_PL011_MIS_OEMIS        (1 << 10)
#define UART_PL011_MIS_BEMIS        (1 << 9)
#define UART_PL011_MIS_PEMIS        (1 << 8)
#define UART_PL011_MIS_FEMIS        (1 << 7)
#define UART_PL011_MIS_RTMIS        (1 << 6)
#define UART_PL011_MIS_TXMIS        (1 << 5)
#define UART_PL011_MIS_RXMIS        (1 << 4)
#define UART_PL011_MIS_DSRMMIS      (1 << 3)
#define UART_PL011_MIS_DCDMMIS      (1 << 2)
#define UART_PL011_MIS_CTSMMIS      (1 << 1)
#define UART_PL011_MIS_RIMMIS       (1 << 0)

#define UART_PL011_ICR_OEIC     (1 << 10)
#define UART_PL011_ICR_BEIC     (1 << 9)
#define UART_PL011_ICR_PEIC     (1 << 8)
#define UART_PL011_ICR_FEIC     (1 << 7)
#define UART_PL011_ICR_RTIC     (1 << 6)
#define UART_PL011_ICR_TXIC     (1 << 5)
#define UART_PL011_ICR_RXIC     (1 << 4)
#define UART_PL011_ICR_DSRMIC       (1 << 3)
#define UART_PL011_ICR_DCDMIC       (1 << 2)
#define UART_PL011_ICR_CTSMIC       (1 << 1)
#define UART_PL011_ICR_RIMIC        (1 << 0)

#define UART_PL01x_DR                   0x00     /*  Data read or written from the interface. */
#define UART_PL01x_RSR                  0x04     /*  Receive status register (Read). */
#define UART_PL01x_ECR                  0x04     /*  Error clear register (Write). */
#define UART_PL01x_FR                   0x18     /*  Flag register (Read only). */

#define UART_PL01x_RSR_OE               0x08
#define UART_PL01x_RSR_BE               0x04
#define UART_PL01x_RSR_PE               0x02
#define UART_PL01x_RSR_FE               0x01

#define UART_PL01x_FR_TXFE              0x80
#define UART_PL01x_FR_RXFF              0x40
#define UART_PL01x_FR_TXFF              0x20
#define UART_PL01x_FR_RXFE              0x10
#define UART_PL01x_FR_BUSY              0x08
#define UART_PL01x_FR_TMSK              (UART_PL01x_FR_TXFF + UART_PL01x_FR_BUSY)

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

    /*
     * Set baud rate
     *
     baudrate = 115200
     UART clock = 24000000
     * IBRD = UART_CLK / (16 * BAUD_RATE)
     * FBRD = RND((64 * MOD(UART_CLK,(16 * BAUD_RATE))) 
     *    / (16 * BAUD_RATE))
     */

    write_uart(UART_PL011_IBRD, 13, uart_id);

    /* Set the UART to be 8 bits, 1 stop bit, 
     * no parity, fifo enabled 
     */
    write_uart(UART_PL011_LCRH, (UART_PL011_LCRH_WLEN_8 | UART_PL011_LCRH_FEN), uart_id);

    /* Finally, enable the UART */
    write_uart(UART_PL011_CR, (UART_PL011_CR_UARTEN | UART_PL011_CR_TXE), uart_id);

    return;
}