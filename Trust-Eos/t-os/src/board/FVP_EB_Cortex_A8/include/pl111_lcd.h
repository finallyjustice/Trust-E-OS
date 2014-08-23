#ifndef _BOARD_FVB_EB_CORTEX_A8_PL111_LCD_H_
#define _BOARD_FVB_EB_CORTEX_A8_PL111_LCD_H_

#include "eb_regs_base.h"

#define LCD_WIDTH       (640)
#define LCD_HEIGHT      (480)
#define RGB(r,g,b)      ((b)<<10 | (g)<<5 | (r))

// PL111寄存器
#define PL111_TIM0          __REG(PL111_CLCD_BASE + 0x00)
#define PL111_TIM1          __REG(PL111_CLCD_BASE + 0x04)
#define PL111_TIM2          __REG(PL111_CLCD_BASE + 0x08)
#define PL111_TIM3          __REG(PL111_CLCD_BASE + 0x0C)
#define PL111_UBAS          __REG(PL111_CLCD_BASE + 0x10)
#define PL111_LBAS          __REG(PL111_CLCD_BASE + 0x14)
#define PL111_CNTL          __REG(PL111_CLCD_BASE + 0x18)
#define PL111_IENB          __REG(PL111_CLCD_BASE + 0x1C)

// 说明：初始化PL111 LCD
// 参数：
//       width
//       height
//       
int init_pl111(int width, int height);
//void clear_screen(uint16_t color);


static inline void draw_pixel(volatile uint8_t* frame_buffer, uint32_t x, 
                        uint32_t y, uint16_t color)
{
    *((volatile uint16_t*)frame_buffer + y*LCD_WIDTH + x) = color; 
}


#endif