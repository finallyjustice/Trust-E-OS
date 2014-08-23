/*
 *  linux/lib/vsprintf.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */
/*
 * Wirzenius wrote this portably, Torvalds fucked it up :-)
 */

#include <stdarg.h>
#include "ctype.h" 
#include "tee_internal_api.h"
#include "tee_debug.h"
#include "tee_string.h"

static char *OS_INFO = "[T-OS INFO:]";

unsigned int TEE_Printf(const char *fmt, ...)
{ 
    va_list args;
    unsigned int i;
    char *p;
    char print_buffer[256];
    va_start(args, fmt);

    ////////////////////////////
    ////////////////////////////
    p = OS_INFO;
    while(*p != '\0') {
        debug_putc(*p);
        p++;
    }
    i = sw_vsprintf(print_buffer, fmt, args);
    va_end(args);

    for (p = print_buffer; *p; p++) {
        if (*p == '\n')
            debug_putc('\r');
        debug_putc(*p);
    }

    /* Print the string */
   //sw_write("UART",print_buffer);
    return i;
}

unsigned int sw_sprintf(char *print_buffer, const char *fmt, ...)
{
	va_list args;
	unsigned int i;
	va_start(args, fmt);

	i = sw_vsprintf(print_buffer, fmt, args);
	va_end(args);

	return i;
}

int sw_vsprintf(char *buf, const char *fmt, va_list args)
{ 
    char *str;
    str = buf;

    for(; *fmt ; ++fmt){
        if((*fmt != '%') && (*fmt != '\n') && (*fmt != '\t')){
            *str++ = *fmt;
            continue;
        }

        if(*fmt == '%'){
            /* skip % */
            ++fmt;
            int is_unsigned = 0;
            int zero_padding = 1;
            
            if(*fmt == '0'){
                /* zero padding!*/
                /* skip 0 */
                ++fmt;
                zero_padding = *fmt++;
                if((zero_padding < 0x31) || (zero_padding > 0x38)){
                    debug_puts("invalid padding bits.\0");
                }
                zero_padding -= 0x30;
            }

            switch(*fmt){
                case 'l':
                {
                    ++fmt;
                    break;
                }
            }
            
            switch(*fmt){       
                case 'x':
                {
                    int number = va_arg(args, int);
                    int length = 8;
                    int length_in_bits = 32;
                    int byte = 0;
                    int i = 0;
                    bool keep_zeros = FALSE;
                
                    for(i = 0; i < length; i++){
                        byte = number >> (length_in_bits - ((i+1) * 4));
                        byte = byte & 0xF;
                        if(byte != 0){
                            keep_zeros = TRUE;
                        }
                        if(keep_zeros || i >= (7-(zero_padding-1))){
                            if((byte >= 0) && (byte <= 9)){
                                byte = byte + 0x30;
                            }
                            else{
                                switch(byte){
                                    case 0xa:
                                        byte = 0x61;
                                        break;
                                    case 0xb:
                                        byte = 0x62;
                                        break;
                                    case 0xc:
                                        byte = 0x63;
                                        break;
                                    case 0xd:
                                        byte = 0x64;
                                        break;
                                    case 0xe:
                                        byte = 0x65;
                                        break;
                                    case 0xf:
                                        byte = 0x66;
                                        break;
                                } /* switch ends */
                            } /* else ends */
                            *str++ = byte;
                        }
                    } /* for ends - whole number is now done */
                    break;
                }
                case 'u':
                    is_unsigned = 1;
                case 'i':
                case 'd':
                {
                    int i,j,max_num_zeros,num_of_digits_u32,number_u32,
                        divisor_value_u32,new_div_val = 1,sw_quotient_value = 0;
                    bool keep_zeros = FALSE;
                     
                    if(!is_unsigned){
                        int signed_num_32 = va_arg(args,int);
                        if(signed_num_32 < 0){
                            *str++ = 0x2d;
                            signed_num_32 = -(signed_num_32);
                        }
                        number_u32 = (int)signed_num_32;
                    }
                    else{
                        int unsigned_value_32 = va_arg(args,unsigned int);
                        number_u32 = unsigned_value_32;
                    }
            
                    divisor_value_u32 = 1000000000;
                    num_of_digits_u32 = 10;
                    max_num_zeros = num_of_digits_u32 - 1;

                    for(i = 0; i < max_num_zeros; i++){
                        while(number_u32 >= divisor_value_u32){
                            number_u32 -= divisor_value_u32;
                            ++sw_quotient_value;
                        }
                        if(sw_quotient_value != 0)
                            keep_zeros = TRUE;
                        if(keep_zeros || i > ((max_num_zeros-1)-(zero_padding-1))){
                            sw_quotient_value += 0x30;
                            *str++ = sw_quotient_value;
                        }
                        j = i;
                        while(j < (max_num_zeros-1)){
                            new_div_val *= 10;
                            j++;
                        }
                        sw_quotient_value = 0;
                        divisor_value_u32 = new_div_val;
                        new_div_val = 1;
                    }
                    *str++ = (number_u32 + 0x30);
                    break;
                }
                case 'o':
                {
                    int number,length = 10,length_in_bits = 32,byte = 0,i = 0;
                    bool keep_zeros = FALSE;

                    number = va_arg(args, int);
                    byte = number >> 30;
                    byte &= 0x3;
                    if(byte != 0){
                        keep_zeros = TRUE;
                    }
                    if(keep_zeros || zero_padding > length){
                        byte = byte + 0x30;
                        *str++ = byte;
                    }

                    number <<= 2;
                    for(i = 0; i < length; i++){
                        byte = number >> (length_in_bits - ((i+1) * 3));
                        byte &= 0x7;                
                        if(byte != 0){
                            keep_zeros = TRUE;
                        }
                        if(keep_zeros || i >= (9-(zero_padding-1))){
                            byte = byte + 0x30;
                            *str++ = byte;
                        }
                    }
                    break;
                }
                case 's':
                {
                    char *arg_string = va_arg(args, char *);
                    while((*str = *arg_string++)){
                        ++str;
                    }   
                    break;
                }
                case 'c':
                {
                    
                    char character = (char)va_arg(args, int);
                    //character = va_arg(args, char);
                    *str++ = character;
                    break;
                }
                case '%':
                {
                    *str++ = *fmt;
                    break;
                }
                case '\t':
                {
                    *str++ = '%';
                    *str++ = *fmt;
                    break;
                }
                case '\n':
                {
                    *str++ = '%';
                    *str++ = '\r';
                    *str++ = '\n';
                    break;
                }
                default:
                {
                    debug_puts("option after %: ");
                    debug_putc(*fmt);
                    debug_puts("\r\n");
                    debug_puts("Unknown option after %\0");
                }
            } /* switch ends             */
        } /* if % character found      */

        if(*fmt == '\n'){
            *str++ = '\r';
            *str++ = '\n';
        }
        if(*fmt == '\t')
            *str++ = *fmt;
    } /* for ends */
    *str = '\0';
    return TEE_StrLen(str); 
}
