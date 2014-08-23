#ifndef _BOARD_FVB_EB_CORTEX_A8_H_EB_REGS_BASE_H_
#define _BOARD_FVB_EB_CORTEX_A8_H_EB_REGS_BASE_H_

#include "tee_internal_api.h"

// EB register base addrss
#define SYS_REG_BASE            (0x10000000)
#define PL111_CLCD_BASE         (0x10020000)
#define PL011_UART0_BASE         (0x10009000)
#define PL011_UART1_BASE         (0x1000A000)
#define PL011_UART2_BASE         (0x1000B000)
#define PL011_UART3_BASE         (0x1000C000)

// EB system registers
#define __REG(x)                (*((volatile uint32_t *)(x)))
#define REG_SYS_ID              __REG(SYS_REG_BASE + 0x0)
#define REG_SYS_SW              __REG(SYS_REG_BASE + 0x4)
#define REG_SYS_LED             __REG(SYS_REG_BASE + 0x8)
#define REG_SYS_OSC0            __REG(SYS_REG_BASE + 0x0C)
#define REG_SYS_OSC1            __REG(SYS_REG_BASE + 0x10)
#define REG_SYS_OSC2            __REG(SYS_REG_BASE + 0x14)
#define REG_SYS_OSC3            __REG(SYS_REG_BASE + 0x18)
#define REG_SYS_OSC4            __REG(SYS_REG_BASE + 0x1C)
#define REG_SYS_LOCK            __REG(SYS_REG_BASE + 0x20)
#define REG_SYS_100HZ           __REG(SYS_REG_BASE + 0x24)
#define REG_SYS_CONFIGDATA1     __REG(SYS_REG_BASE + 0x28)
#define REG_SYS_CONFIGDATA2     __REG(SYS_REG_BASE + 0x2C)
#define REG_SYS_FLAGS           __REG(SYS_REG_BASE + 0x30)
#define REG_SYS_FLAGSCLR        __REG(SYS_REG_BASE + 0x34)
#define REG_SYS_NVFLAGS         __REG(SYS_REG_BASE + 0x38)
#define REG_SYS_NVFLAGSCLR      __REG(SYS_REG_BASE + 0x3C)
#define REG_SYS_PCICTL          __REG(SYS_REG_BASE + 0x44)
#define REG_SYS_MCI             __REG(SYS_REG_BASE + 0x48)
#define REG_SYS_FLASH           __REG(SYS_REG_BASE + 0x4C)
#define REG_SYS_CLCD            __REG(SYS_REG_BASE + 0x50)
#define REG_SYS_CLCDSER         __REG(SYS_REG_BASE + 0x54)
#define REG_SYS_BOOTCS          __REG(SYS_REG_BASE + 0x58)

#endif