#include "monitor.h"
#include "cpu.h"
#include "tee_debug.h"
#include "tee_internal_api.h"
#include "cpu_mmu.h"

/**
 * @brief Prefetch abort fault strings
 */
static const char* prefetch_abt_fault_string[] =
{
  "INVALID ENTRY",                            /*  0 = 0b00000, */
  "INVALID ENTRY",                            /*  1 = 0b00001, */
  "Debug Event",                              /*  2 = 0b00010, */
  "Access Flag Fault: Section",               /*  3 = 0b00011, */
  "INVALID ENTRY",                            /*  4 = 0b00100, */
  "Translation Fault: Section",               /*  5 = 0b00101, */
  "Access Flag Fault: Page",                  /*  6 = 0b00110, */
  "Translation Fault: Page",                  /*  7 = 0b00111, */
  "Synchronous External Abort",               /*  8 = 0b01000, */
  "Domain Fault: Section",                    /*  9 = 0b01001, */
  "INVALID ENTRY",                            /*  a = 0b01010, */
  "Domain Fault: Page",                       /*  b = 0b01011, */
  "Translation Table Talk 1st Level sync abt",/*  c = 0b01100, */
  "Permission Fault: Section",                 /*  d = 0b01101, */
  "Translation Table Walk 2nd Level sync abt",/*  e = 0b01110, */
  "Permission Fault: Page",                   /*  f = 0b01111, */
  "INVALID ENTRY",                            /* 10 = 0b10000, */
  "INVALID ENTRY",                            /* 11 = 0b10001, */
  "INVALID ENTRY",                            /* 12 = 0b10010, */
  "INVALID ENTRY",                            /* 13 = 0b10011, */
  "IMPLEMENTATION DEFINED Lockdown",          /* 14 = 0b10100, */
  "INVALID ENTRY",                            /* 15 = 0b10101, */
  "INVALID ENTRY",                            /* 16 = 0b10110, */
  "INVALID ENTRY",                            /* 17 = 0b10111, */
  "INVALID ENTRY",                            /* 18 = 0b11000, */
  "Memory Access Synchronous Parity Error",   /* 19 = 0b11001, */
  "IMPLEMENTATION DEFINED Coprocessor Abort", /* 1a = 0b11010, */
  "INVALID ENTRY",                            /* 1b = 0b11011, */
  "Translation Table Walk 1st Level parity",  /* 1c = 0b11100, */
  "INVALID ENTRY",                            /* 1d = 0b11101, */
  "Translation Table Walk 2nd Level parity",  /* 1e = 0b11110, */
  "INVALID ENTRY",                            /* 1f = 0b11111, */
};

struct read_only_mem{
    uint32_t phys_addr;
    int value;
};

static struct read_only_mem ro_mem[]={
    { .phys_addr = 0x40,
      .value = 0xe59f0124
    },
    { .phys_addr = 0xfa200fe0, 
      .value = 48
    },
    { .phys_addr = 0xfa200fe4, 
      .value = 19
    },
    { .phys_addr = 0xfa200fe8, 
      .value = 4
    },
    { .phys_addr = 0xfa200fec, 
      .value = 0
    },
    { .phys_addr = 0xfa200ff0, 
      .value = 13
    },
    { .phys_addr = 0xfa200ff4, 
      .value = 240
    },
    { .phys_addr = 0xfa200ff8, 
      .value = 5
    },
    { .phys_addr = 0xfa200ffc, 
      .value = 177
    },
    { .phys_addr = 0xfa200e14, 
      .value = 66533171
    },
    { .phys_addr = 0xfa200e00, 
      .value = 4067441
    },
    { .phys_addr = 0xfa200e10, 
      .value = 3
    },
    { .phys_addr = 0xfa200e0c, 
      .value = -1
    }
};


void monitor_data_abort_c_handler(OS_CONTEXT *context)
{
    /* 读DFSR (Data Fault Status Register) */
    uint32_t dfsr = read_dfsr();
    /* 读DFAR，读取的地址为Linux下的虚拟地址 */
    uint32_t dfar = read_dfar();
    /* 从dfsr中读取第[3:0]、[10]、[12]位，这6位表示数据异常类型，具体类型
     * 见Cortex -A8 Technical Reference Manual p3-84
     */
    uint32_t fault_status = (dfsr & 0xF) | 
                       ((dfsr & 0x400) >> 6) | 
                       ((dfsr & 0x1000) >> 7);
    uint32_t pa_dfar;
    int is_ro_mem = 0, i;
    //dfar = 0xd8818fe0;
    TEE_Printf("Prefetch Abort Address vir: %08x\n", (dfar));
    TEE_Printf("Prefetch Abort Address phy: %08x\n", va_to_pa_ns(dfar));
    TEE_Printf("Fault type: %s \n",
          (char*)prefetch_abt_fault_string[fault_status]); 
    
    /* 检查是否是外部数据终止 */
    if(fault_status == 0x8){
        /* 计算导致异常的寄存器的物理地址 */
        pa_dfar = va_to_pa_ns(dfar);
        /* 检查异常是否是由DMA_mem的只读寄存器引起的 */
        for(i = 0; i < sizeof(ro_mem)/sizeof(struct read_only_mem); i++){
            if(pa_dfar == ro_mem[i].phys_addr){
                is_ro_mem = 1;
                break;
            }
        }
        /* 如果异常是由DMA_mem的只读寄存器引起的 */
        if(is_ro_mem == 1){
            /* 从非安全模式上下文堆栈中读取引起异常的那条指令的
             * 下一条指令的地址， 并将其转换成物理地址，然后减4
             * 得到引起异常的那条指令的物理地址。
             */
            uint32_t lr_pa = va_to_pa_ns(context->pc) - 4; 
            uint32_t instruction;
            uint32_t rd;
            char op;
            /*读取引起异常的那条指令,并根据ARM指令集编码(见<ARM
             * 嵌入式系统开发-软件设计与优化>附录B)从中提取出目
             * 标寄存器和操作码。
             */
            instruction = *((uint32_t *)lr_pa);
            rd = (instruction & 0x0000f000) >> 12;
            op = ((instruction & 0x00100000)>>20) | 
                ((instruction & 0x00400000) >> 21);
            if(op == 3){//LDRB
                //regs->regs[rd] = *(volatile char *)pa_dfar;
                *((uint32_t*)((uint32_t)context + 4*rd)) =  *(volatile char *)pa_dfar;
            }
            else if(op == 1)//LDR
            {
                //regs->regs[rd] = *(volatile uint32_t *)pa_dfar;
                *((uint32_t*)((uint32_t)context + 4*rd)) =  *(volatile uint32_t *)pa_dfar;
            }
            else
               while(1);
            //TEE_Printf("lr = %lx\n",lr_pa);
            //TEE_Printf("pa_dfar = %lx,value = %lx\n",pa_dfar,regs->regs[rd]);
            TEE_Printf("pa_dfar = %lx,value = %lx\n",pa_dfar,*((uint32_t*)((uint32_t)context + 4*rd)));
        }
        else
        {
            TEE_Printf("hang\n");
            while(1);
        }
    }
    else
    {
        TEE_Printf("hang\n");
        while(TRUE);
    } 
}