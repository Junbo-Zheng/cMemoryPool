#ifndef _MALLOC_H
#define _MALLOC_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define INSRAM
#define EXTRAM
#define CCMRAM
#define EXTRAM
#define EXTRAM

//定义5个内存池
#define SRAMIN 	0  // 内部内存池
#define SRAMEX 	1  // 外部内存池
#define SRAMCCM 2  // CCM内存池
#define SRAMEX1 3  // 外部内存池
#define SRAMEX2 4  // 外部内存池
#define SRAMNONE 0xff

#define SRAMBANK  5 // 定义支持的SRAM块数

// mem1内存参数设定,mem1完全处于内部SRAM里面
#define MEM1_BLOCK_SIZE       32                             // 内存块大小为32字节
#define MEM1_MAX_SIZE         100*1024                       // 最大管理内存 100k
#define MEM1_ALLOC_TABLE_SIZE MEM1_MAX_SIZE/MEM1_BLOCK_SIZE  // 内存表大小

// mem2内存参数设定,mem2处于外部SRAM里面
#define MEM2_BLOCK_SIZE        32                            // 内存块大小为32字节
#define MEM2_MAX_SIZE          170*1024                      // 最大管理内存 170k
#define MEM2_ALLOC_TABLE_SIZE MEM2_MAX_SIZE/MEM2_BLOCK_SIZE  // 内存表大小

// mem3内存参数设定,mem3处于CCM,用于管理CCM(特别注意,这部分SRAM,近CPU可以访问)
#define MEM3_BLOCK_SIZE        32                            // 内存块大小为32字节
#define MEM3_MAX_SIZE          32                            // 最大管理内存 32字节
#define MEM3_ALLOC_TABLE_SIZE MEM3_MAX_SIZE/MEM3_BLOCK_SIZE  // 内存表大小

// mem4内存参数设定,mem4处于外部SRAM里面
#define MEM4_BLOCK_SIZE        32                            // 内存块大小为32字节
#define MEM4_MAX_SIZE          50*1024                       // 最大管理内存 50k
#define MEM4_ALLOC_TABLE_SIZE MEM4_MAX_SIZE/MEM4_BLOCK_SIZE  // 内存表大小

// mem5内存参数设定,mem5处于外部SRAM里面
#define MEM5_BLOCK_SIZE        32                            // 内存块大小为32字节
#define MEM5_MAX_SIZE          50*1024                       // 最大管理内存 50k
#define MEM5_ALLOC_TABLE_SIZE MEM5_MAX_SIZE/MEM5_BLOCK_SIZE  // 内存表大小

#if ((MEM1_MAX_SIZE % 32) ? (1): (0))
#error "MEM1_MAX_SIZE must be a power of two"
#endif

#if ((MEM2_MAX_SIZE % 32) ? (1) : (0))
#error "MEM2_MAX_SIZE must be a power of two"
#endif

#if ((MEM3_MAX_SIZE % 32) ? (1) : (0))
#error "MEM3_MAX_SIZE must be a power of two"
#endif

#if ((MEM4_MAX_SIZE % 32) ? (1) : (0))
#error "MEM4_MAX_SIZE must be a power of two"
#endif

#if ((MEM5_MAX_SIZE % 32) ? (1) : (0))
#error "MEM1_MAX_SIZE must be a power of two"
#endif

#define MYMALLOC(memx, size)       mymalloc((memx), (size), __FILE__, __LINE__)
#define MYFREE(ptr)                myfree((ptr), __FILE__, __LINE__)

// 内存管理控制器
struct _m_mallco_dev {
    void (*init)(uint8_t);       // 初始化
    uint8_t (*perused)(uint8_t); // 内存使用率
    uint8_t* membase[SRAMBANK];  // 内存池,管理SRAMBANK个区域的内存
    uint16_t* memmap[SRAMBANK];  // 内存状态（管理）表
    uint8_t memrdy[SRAMBANK];    // 内存管理是否就绪
};

void mymem_init(uint8_t memx);   // 内存管理初始化函数(外/内部调用)

void *mymalloc(uint8_t memx, uint32_t size, char* file_name, uint32_t func_line); // 内存分配(外部调用)
void myfree(void *ptr, char* file_name, uint32_t func_line);                      // 内存释放(外部调用)

void mymemset(void* s, uint8_t c, uint32_t count);
void mymemcpy(void* des, void* src, uint32_t n);
//void *myrealloc(uint8_t memx,void *ptr,uint32_t size);// 重新分配内存(外部调用)

uint8_t mem_perused(uint8_t memx); // 获得内存使用率(外/内部调用)

#endif
