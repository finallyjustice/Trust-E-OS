#include "tee_internal_api.h"
#include "tee_mem_mng.h"
#include "tee_string.h"
#include "tee_mmf.h"

extern uint32_t _TZ_CODE_START;
extern uint32_t _TZ_CODE_END;

typedef struct _TEE_MEM_INFO {
    // physical start address
    uint32_t phy_start_addr;
    // physical end address
    uint32_t phy_end_addr;
    // virtual start address
    uint32_t vir_start_addr;
    // virtual end address
    uint32_t vir_end_addr;
    // total physical memory 所有的物理内存
    uint32_t total_phy_size;
    // code range
    uint32_t phy_code_start_addr;
    uint32_t phy_code_end_addr;
    uint32_t vir_code_start_addr;
    uint32_t vir_code_end_addr;
    // physical memory bmp(物理内存位图)
    void *phy_mem_bmp;
    // uint:byte (单位：字节)
    uint32_t phy_mem_bmp_len; 
    // uint:byte (第一块可以用的物理内存位图索引)
    uint32_t first_valid_phy_bmp_index;
}TEE_MEM_INFO;

// tee world memory information
TEE_MEM_INFO g_tee_mem_info;

// 初始化位图
static void TEE_InitMemBmp();

// 位图置位
static inline void TEE_BmpSet(uint32_t page_index)
{
    uint8_t *bmp = (uint8_t *)g_tee_mem_info.phy_mem_bmp;
    bmp[page_index>>3] |= 1 << (page_index & 0x7);
}

// 位图清位
static inline void TEE_BmpClear(uint32_t page_index)
{
    uint8_t *bmp = (uint8_t *)g_tee_mem_info.phy_mem_bmp;
    bmp[page_index>>3] &= ~(1 << (page_index & 0x7));
}

// 位图是否被占用
static inline bool TEE_BmpTest(uint32_t page_index)
{
    uint8_t *bmp = (uint8_t *)g_tee_mem_info.phy_mem_bmp;
    return (bmp[page_index >> 3] & (1 << (page_index & 0x7))) != 0 ? TRUE : FALSE;
}

// 位图对应地址
static inline uint32_t TEE_BmpAddr(uint32_t page_index)
{ 
    return g_tee_mem_info.phy_start_addr + (page_index << TEE_PAGE_SHIFT);
}

// 位图 地图转换为页索引
// 返回值 失败返回-1，否则返回索引
static inline int32_t TEE_Addr2PageIndex(void *addr)
{ 
    if (((uint32_t)addr & (TEE_PAGE_SIZE -1)) == 0) {
        return ((uint32_t)addr - (uint32_t)g_tee_mem_info.phy_start_addr) >> TEE_PAGE_SHIFT; 
    }
    return -1;
}


// 内存初始化
void TEE_MemInit(void)
{
    uint32_t phy_start_addr,phy_end_addr;
    uint32_t vir_start_addr,vir_end_addr;
    uint32_t total_phy_size;
    
    phy_start_addr = (uint32_t)TEE_PHY_MEM_START;
    phy_end_addr = (uint32_t)TEE_PHY_MEM_END;
    vir_start_addr = phy_start_addr;
    vir_end_addr = phy_end_addr;
    total_phy_size = phy_end_addr - phy_start_addr + 1;

    // 赋值结构体
    TEE_MemFill(&g_tee_mem_info, 0, sizeof(g_tee_mem_info));
    g_tee_mem_info.phy_start_addr = phy_start_addr;
    g_tee_mem_info.phy_end_addr = phy_end_addr;
    g_tee_mem_info.vir_start_addr = vir_start_addr;
    g_tee_mem_info.vir_end_addr = vir_end_addr;
    g_tee_mem_info.total_phy_size = total_phy_size;
    g_tee_mem_info.phy_code_start_addr = (uint32_t)&_TZ_CODE_START;
    g_tee_mem_info.phy_code_end_addr = (uint32_t)&_TZ_CODE_END;
    g_tee_mem_info.vir_code_start_addr = (uint32_t)&_TZ_CODE_START;
    g_tee_mem_info.vir_code_end_addr = (uint32_t)&_TZ_CODE_END;

    TEE_InitMemBmp();
}

// alloc pages
// 采用首次适应算法
void *TEE_AllocPages(uint32_t num)
{
    uint32_t index = g_tee_mem_info.first_valid_phy_bmp_index;
    uint32_t t_num = 0;
    uint32_t addr = 0;
    while (t_num < num && (index >> 3) < g_tee_mem_info.phy_mem_bmp_len) {
        if (TEE_BmpTest(index) == FALSE) {
            t_num++;
        }
        else {
            t_num = 0;
        }
        index++;
    }
    if (t_num != num) {
        return NULL;
    }
    index -= num;
    // 更新第一块可用的page索引
    g_tee_mem_info.first_valid_phy_bmp_index = index;
    addr = TEE_BmpAddr(index);
    // update bmp
    while (num--) {
        TEE_BmpSet(index++);
    }

    // return the address of page
    return (void*)addr;
}

void TEE_FreePages(void *addr, uint32_t num)
{
    int32_t page_index = TEE_Addr2PageIndex(addr);
    if (page_index != -1) {
        // 更新first_valid_page_index
        if (g_tee_mem_info.first_valid_phy_bmp_index > page_index) {
            g_tee_mem_info.first_valid_phy_bmp_index = page_index;
        }
        while (num--) {
            TEE_BmpClear(page_index++);
        }
    }
}

// 初始化位图
static void TEE_InitMemBmp()
{
    uint32_t page_index = 0;
    uint32_t page_count = 0;
    uint32_t start_addr,end_addr;
    page_count = (g_tee_mem_info.total_phy_size & TEE_PAGE_SIZE) == 0 ?
                (g_tee_mem_info.total_phy_size >> TEE_PAGE_SHIFT) :
                (((g_tee_mem_info.total_phy_size >> TEE_PAGE_SHIFT)) + 1);

    // physical memory bmp length = total_phy_size / page_size + 1
    g_tee_mem_info.phy_mem_bmp_len = (page_count & 0x7) == 0 ?
                                (page_count >> 3) : ((page_count >> 3) + 1);
    // bmp address (align 4)
    g_tee_mem_info.phy_mem_bmp = (g_tee_mem_info.phy_code_end_addr & 0x3) == 0 ?
                                 (void *)g_tee_mem_info.phy_code_end_addr :
                                 (void*)((g_tee_mem_info.phy_code_end_addr & 0x3) + 4);
    // used bmp
    uint8_t *bmp = (uint8_t *)g_tee_mem_info.phy_mem_bmp;

    // init bmp 
    TEE_MemFill(bmp, 0, g_tee_mem_info.phy_mem_bmp_len);
    start_addr = g_tee_mem_info.phy_code_start_addr;
    end_addr = (uint32_t)g_tee_mem_info.phy_mem_bmp + g_tee_mem_info.phy_mem_bmp_len;
    page_index = 0;
    while (start_addr < end_addr + TEE_PAGE_SIZE) {
        // 第page_index页对应的位图赋值
        //bmp[page_index>>3] |= 1 << (page_index & 0x7);
        TEE_BmpSet(page_index);
        start_addr += TEE_PAGE_SIZE;
        page_index++;
    }

    // update first_valid_phy_bmp_index
    g_tee_mem_info.first_valid_phy_bmp_index = page_index;
}
