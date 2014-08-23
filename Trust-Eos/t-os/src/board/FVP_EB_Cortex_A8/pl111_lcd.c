#include "pl111_lcd.h"
#include "tee_internal_api.h"


// frame buffer
uint8_t  frame_buffer[LCD_WIDTH * LCD_HEIGHT * 2];
 



//! Configure PL111 CLCD
int init_pl111(int width, int height)
{
            // Timing number for an 8.4" LCD screen for use on a VGA screen
    uint32_t TIM0_VAL = ( (((width/16)-1)<<2) | (63<<8) | (31<<16) | (63<<8) );
    uint32_t TIM1_VAL = ( (height - 1) | (24<<10) | (11<<16) | (9<<24) );
    uint32_t TIM2_VAL = ( (0x7<<11) | ((width - 1)<<16) | (1<<26) );


         // Program the CLCD controller registers and start the CLCD
    PL111_TIM0 = TIM0_VAL;
    PL111_TIM1 = TIM1_VAL;
    PL111_TIM2 = TIM2_VAL;
    PL111_TIM3 = 0;
    PL111_UBAS = (uint32_t)frame_buffer;
    PL111_LBAS = 0;
    PL111_IENB = 0;

         // Set the control register: 16BPP, Power OFF
    PL111_CNTL = (1<<0) | (4<<1) | (1<<5);

         // Power ON
    PL111_CNTL |= (1<<11);
    return 0;
}
 

void clear_screen(uint16_t color)
{
    volatile uint32_t x,y;
    for (y=0; y<LCD_HEIGHT/2; y++) {
        for (x=0; x<LCD_WIDTH; x++) {
            draw_pixel(frame_buffer, x, y, color);
        }
    }
}
