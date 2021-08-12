#include "malloc.h"

static uint32_t mymem_malloc(uint8_t memx, uint32_t size); //内存分配(内部调用)

static uint8_t mymem_free(uint8_t memx, uint32_t offset);  //内存释放(内部调用)

//内存池(4字节对齐)
#pragma pack(4)
INSRAM /*__align(4)*/ uint8_t mem1base[MEM1_MAX_SIZE];//内部SRAM内存池
EXTRAM /*__align(4)*/ uint8_t mem2base[MEM2_MAX_SIZE];//外部SRAM内存池
CCMRAM /*__align(4)*/ uint8_t mem3base[MEM3_MAX_SIZE];//内部CMM内存池
EXTRAM /*__align(4)*/ uint8_t mem4base[MEM4_MAX_SIZE];//外部SRAM内存池
EXTRAM /*__align(4)*/ uint8_t mem5base[MEM5_MAX_SIZE];//外部SRAM内存池
#pragma pack()

//内存管理表
INSRAM uint16_t mem1mapbase[MEM1_ALLOC_TABLE_SIZE];//内部SRAM内存池MAP
EXTRAM uint16_t mem2mapbase[MEM2_ALLOC_TABLE_SIZE];//外部SRAM内存池MAP
CCMRAM uint16_t mem3mapbase[MEM3_ALLOC_TABLE_SIZE];//内部CCM内存池MAP
EXTRAM uint16_t mem4mapbase[MEM4_ALLOC_TABLE_SIZE];//外部SRAM内存池MAP
EXTRAM uint16_t mem5mapbase[MEM5_ALLOC_TABLE_SIZE];//外部SRAM内存池MAP

//内存管理参数
//内存表大小
const uint32_t memtblsize[SRAMBANK]
    = { MEM1_ALLOC_TABLE_SIZE, MEM2_ALLOC_TABLE_SIZE, MEM3_ALLOC_TABLE_SIZE,
        MEM4_ALLOC_TABLE_SIZE, MEM5_ALLOC_TABLE_SIZE };

//内存分块大小
const uint32_t memblksize[SRAMBANK]
    = { MEM1_BLOCK_SIZE, MEM2_BLOCK_SIZE, MEM3_BLOCK_SIZE, MEM4_BLOCK_SIZE,
        MEM5_BLOCK_SIZE };

//内存大小
const uint32_t memsize[SRAMBANK]
    = { MEM1_MAX_SIZE, MEM2_MAX_SIZE, MEM3_MAX_SIZE, MEM4_MAX_SIZE,
        MEM5_MAX_SIZE };

//内存管理控制器
struct _m_mallco_dev mallco_dev
    = { mymem_init,  //内存初始化
        mem_perused, //内存使用率
        //内存池
        { mem1base, mem2base, mem3base, mem4base, mem5base },
        //内存管理状态表
        { mem1mapbase, mem2mapbase, mem3mapbase, mem4mapbase, mem5mapbase },
        //内存管理未就绪
        { 0, 0, 0, 0, 0 } };

void mymemcpy(void* des, void* src, uint32_t n)
{
    uint8_t* xdes = des;
    uint8_t* xsrc = src;
    while (n--)
        *xdes++ = *xsrc++;
}

void mymemset(void* s, uint8_t c, uint32_t count)
{
    uint8_t* xs = s;
    while (count--)
        *xs++ = c;
}

//内存管理初始化
//memx:所属内存块
void mymem_init(uint8_t memx)
{
    mymemset(mallco_dev.memmap[memx], 0, memtblsize[memx] * 2); //内存状态表清零
    mymemset(mallco_dev.membase[memx], 0, memsize[memx]);       //内存池所有数据清零
    mallco_dev.memrdy[memx] = 1; //内存管理初始化OK
}

//获取内存使用率
//memx:所属内存块
//返回值:使用率(0~100)
uint8_t mem_perused(uint8_t memx)
{
    uint32_t used = 0;

    for (uint32_t i = 0; i < memtblsize[memx]; i++) {
        if (mallco_dev.memmap[memx][i])
            used++;
    }

    return (used * 100) / (memtblsize[memx]);
}

//内存分配(内部调用)
//memx:所属内存块
//size:要分配的内存大小(字节)
//返回值:0XFFFFFFFF,代表错误;其他,内存偏移地址
static uint32_t mymem_malloc(uint8_t memx, uint32_t size)
{
    if (size == 0)
        return 0XFFFFFFFF; //不需要分配

    if (!mallco_dev.memrdy[memx])
        mallco_dev.init(memx); //未初始化,先执行初始化

    signed long offset = 0;
    uint16_t nmemb;     //需要的内存块数
    uint16_t cmemb = 0; //连续空内存块数

    nmemb = size / memblksize[memx]; //获取需要分配的连续内存块数
    if (size % memblksize[memx])
        nmemb++;

    //搜索整个内存控制区
    for (offset = memtblsize[memx] - 1; offset >= 0; offset--) {
        //连续空内存块数增加
        if (!mallco_dev.memmap[memx][offset])
            cmemb++;
        else
            cmemb = 0; //连续内存块清零

        //找到了连续nmemb个空内存块
        if (cmemb == nmemb) {
            //标注内存块非空
            for (uint32_t i = 0; i < nmemb; i++) {
                mallco_dev.memmap[memx][offset + i] = nmemb;
            }

            //返回偏移地址
            return (offset * memblksize[memx]);
        }
    }
    return 0XFFFFFFFF; //未找到符合分配条件的内存块
}

//释放内存(内部调用)
//memx:所属内存块
//offset:内存地址偏移
//返回值:0,释放成功;1,释放失败;
static uint8_t mymem_free(uint8_t memx,uint32_t offset)
{
	//未初始化,先执行初始化
    if (!mallco_dev.memrdy[memx]) {
        mallco_dev.init(memx);
        return 1; //未初始化
    }

    //偏移在内存池内
    if (offset < memsize[memx]) {

        int index = offset / memblksize[memx]; //偏移所在内存块号码
        int nmemb = mallco_dev.memmap[memx][index]; //内存块数量

        //内存块清零
        for (int i = 0; i < nmemb; i++) {
            mallco_dev.memmap[memx][index + i] = 0;
        }

        return 0;
    }

    return 2; //偏移超区了
}

void myfree(void* ptr, char* file_name, uint32_t func_line)
{
    if (NULL == ptr) {
        return;
    }

    uint32_t offset;
    uint32_t ptr_data = (uint32_t)ptr;

    uint8_t memx = SRAMNONE;

    if ((ptr_data >= (uint32_t)mem1base) && ((ptr_data < (uint32_t)mem1base + MEM1_MAX_SIZE))) {
        memx = SRAMIN;
    } else if ((ptr_data >= (uint32_t)mem2base) && ((ptr_data < (uint32_t)mem2base + MEM2_MAX_SIZE))) {
        memx = SRAMEX;
    } else if ((ptr_data >= (uint32_t)mem3base) && ((ptr_data < (uint32_t)mem3base + MEM3_MAX_SIZE))) {
        memx = SRAMCCM;
    } else if ((ptr_data >= (uint32_t)mem4base) && ((ptr_data < (uint32_t)mem4base + MEM4_MAX_SIZE))) {
        memx = SRAMEX1;
    } else if ((ptr_data >= (uint32_t)mem5base) && ((ptr_data < (uint32_t)mem5base + MEM5_MAX_SIZE))) {
        memx = SRAMEX2;
    } else {
        memx = SRAMNONE;
    }

    if (memx != SRAMNONE) {
        offset = (uint32_t)ptr - (uint32_t)mallco_dev.membase[memx];
        mymem_free(memx, offset); //释放内存
    }

    ptr = NULL;
}

void* mymalloc(uint8_t memx, uint32_t size, char* file_name, uint32_t func_line)
{
    void* tReturn   = NULL;
    uint32_t offset = mymem_malloc(memx, size);

    if (offset != 0XFFFFFFFF) {
        tReturn = (void*)((uint32_t)mallco_dev.membase[memx] + offset);
    }

    return tReturn;
}

//重新分配内存(外部调用)
//memx:所属内存块
//*ptr:旧内存首地址
//size:要分配的内存大小(字节)
//返回值:新分配到的内存首地址
#if 0
void* myrealloc(uint8_t memx, void* ptr, uint32_t size)
{
    void* tReturn = NULL;
    uint32_t offset = mymem_malloc(memx, size);

    if (offset != 0XFFFFFFFF) {
        mymemcpy((void*)((uint32_t)mallco_dev.membase[memx] + offset), ptr, size); //拷贝旧内存内容到新内存
        myfree(memx, ptr);    //释放旧内存
        ptr = NULL;
        tReturn = (void*)((uint32_t)mallco_dev.membase[memx] + offset); //返回新内存首地址
    }

    return tReturn;
}
#endif
