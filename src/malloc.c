#include "malloc.h"

#include <stddef.h>
#include <stdio.h>

#include <pthread.h>

#define MEMPOOL_INIT_READY 0
#define MEMPOOL_INIT_DONE  1

static INSRAM ALIGN_SIZE uint8_t mem1pool[MEM1_POOL_SIZE] = { 0 };
static EXTRAM ALIGN_SIZE uint8_t mem2pool[MEM2_POOL_SIZE] = { 0 };
static CCMRAM ALIGN_SIZE uint8_t mem3pool[MEM3_POOL_SIZE] = { 0 };
static EXTRAM ALIGN_SIZE uint8_t mem4pool[MEM4_POOL_SIZE] = { 0 };
static EXTRAM ALIGN_SIZE uint8_t mem5pool[MEM5_POOL_SIZE] = { 0 };

static INSRAM uint16_t mem1table[MEM1_TABLE_SIZE] = { 0 };
static EXTRAM uint16_t mem2table[MEM2_TABLE_SIZE] = { 0 };
static CCMRAM uint16_t mem3table[MEM3_TABLE_SIZE] = { 0 };
static EXTRAM uint16_t mem4table[MEM4_TABLE_SIZE] = { 0 };
static EXTRAM uint16_t mem5table[MEM5_TABLE_SIZE] = { 0 };

static const uint32_t memtablesize[SRAMBANK] = {
    MEM1_TABLE_SIZE,
    MEM2_TABLE_SIZE,
    MEM3_TABLE_SIZE,
    MEM4_TABLE_SIZE,
    MEM5_TABLE_SIZE
};

static const uint32_t memblocksize[SRAMBANK] = {
    MEM1_BLOCK_SIZE,
    MEM2_BLOCK_SIZE,
    MEM3_BLOCK_SIZE,
    MEM4_BLOCK_SIZE,
    MEM5_BLOCK_SIZE
};

static const uint32_t mempoolsize[SRAMBANK] = {
    MEM1_POOL_SIZE,
    MEM2_POOL_SIZE,
    MEM3_POOL_SIZE,
    MEM4_POOL_SIZE,
    MEM5_POOL_SIZE
};

static struct  {
    void      (*init)(uint8_t);
    uint8_t   (*perused)(uint8_t);
    uint8_t*  mempool[SRAMBANK];
    uint16_t* memtable[SRAMBANK];
    uint8_t   memready[SRAMBANK];
} malloc_dev = {
    mymem_init,

    mem_perused,

    mem1pool,
    mem2pool,
    mem3pool,
    mem4pool,
    mem5pool,

    mem1table,
    mem2table,
    mem3table,
    mem4table,
    mem5table,

    MEMPOOL_INIT_READY,
    MEMPOOL_INIT_READY,
    MEMPOOL_INIT_READY,
    MEMPOOL_INIT_READY,
    MEMPOOL_INIT_READY
};

static pthread_mutex_t mutex[SRAMBANK] = { 0 };

static void mutex_creat(uint8_t memx)
{
#if __linux__
    pthread_mutex_init(&mutex[memx], NULL);
#endif
}

static void mutex_lock(uint8_t memx)
{
#if __linux__
    pthread_mutex_lock(&mutex[memx]);
#endif
}

static void mutex_unlock(uint8_t memx)
{
#if __linux__
    pthread_mutex_unlock(&mutex[memx]);
#endif
}

static uint32_t mymem_malloc(uint8_t memx, uint32_t size)
{
    if (malloc_dev.memready[memx] == MEMPOOL_INIT_READY) {
        malloc_dev.init(memx);
    }

    if (size == 0) {
        return 0xffffffff;
    }

    uint16_t need_block_count = size / memblocksize[memx];
    if (size % memblocksize[memx]) {
        need_block_count++;
    }

    uint16_t empty_block_size = 0;
    for (int32_t offset = (memtablesize[memx] - 1); offset >= 0; offset--) {
        if (offset < 0) {
            return 0xffffffff;
        }

        empty_block_size = (malloc_dev.memtable[memx][offset] == 0)
                               ? (empty_block_size + 1)
                               : 0;

        if (empty_block_size == need_block_count) {
            for (uint32_t i = 0; i < need_block_count; i++) {
                malloc_dev.memtable[memx][offset + i] = need_block_count;
            }
            /* offset address */
            return (offset * memblocksize[memx]);
        }
    }

    return 0xffffffff;
}

static uint8_t mymem_free(uint8_t memx, uint32_t offset)
{
    if (!malloc_dev.memready[memx]) {
        malloc_dev.init(memx);
        return 1;
    }

    if (offset < mempoolsize[memx]) {
        int index = offset / memblocksize[memx];
        int nmemb = malloc_dev.memtable[memx][index];
        for (uint16_t i = 0; i < nmemb; i++) {
            malloc_dev.memtable[memx][index + i] = 0;
        }
        return 0;
    }
    return 2;
}

void mymemcpy(void* des, void* src, uint32_t n)
{
    uint8_t* p_des = des;
    uint8_t* p_src = src;
    while (n--) {
        *p_des++ = *p_src++;
    }
}

void mymemset(void* s, uint8_t c, uint32_t count)
{
    uint8_t* p_des = s;
    while (count--) {
        *p_des++ = c;
    }
}

void mymem_init(uint8_t memx)
{
    mymemset(malloc_dev.memtable[memx], 0,
             memtablesize[memx] * sizeof(uint16_t));
    mymemset(malloc_dev.mempool[memx], 0, mempoolsize[memx]);
    malloc_dev.memready[memx] = MEMPOOL_INIT_DONE;

    mutex_creat(memx);

#if CONFIG_MEMORY_POOL_DEBUG
    init_tracer();
#endif
}

uint8_t mem_perused(uint8_t memx)
{
    uint32_t used = 0;
    for (uint32_t i = 0; i < memtablesize[memx]; i++) {
        if (malloc_dev.memtable[memx][i]) {
            used++;
        }
    }

    return (used * 100) / (memtablesize[memx]);
}

void myfree(void* ptr, char* file_name, uint32_t func_line)
{
    uint32_t ptr_data = (uintptr_t)ptr;

    uint8_t memx = 0xff;
    if (ptr != NULL) {
        if ((ptr_data >= (uintptr_t)mem1pool)
            && ((ptr_data < (uintptr_t)mem1pool + MEM1_POOL_SIZE)))
            memx = SRAMIN;
        else if ((ptr_data >= (uintptr_t)mem2pool)
                 && ((ptr_data < (uintptr_t)mem2pool + MEM2_POOL_SIZE)))
            memx = SRAMEX;
        else if ((ptr_data >= (uintptr_t)mem3pool)
                 && ((ptr_data < (uintptr_t)mem3pool + MEM3_POOL_SIZE)))
            memx = SRAMCCM;
        else if ((ptr_data >= (uintptr_t)mem4pool)
                 && ((ptr_data < (uintptr_t)mem4pool + MEM4_POOL_SIZE)))
            memx = SRAMEX1;
        else if ((ptr_data >= (uintptr_t)mem5pool)
                 && ((ptr_data < (uintptr_t)mem5pool + MEM5_POOL_SIZE)))
            memx = SRAMEX2;
        else
            memx = 0xff;
    }

    if (memx != 0xff) {
        mutex_lock(memx);

        uint32_t offset = (uintptr_t)ptr - (uintptr_t)malloc_dev.mempool[memx];
        mymem_free(memx, offset);
#if CONFIG_MEMORY_POOL_DEBUG
        del_a_tracer_record(ptr, get_filename(file_name), func_line);
#endif
        mutex_unlock(memx);
    }

    ptr = NULL;
}

void* mymalloc(uint8_t memx, uint32_t size, char* file_name, uint32_t func_line)
{
    mutex_lock(memx);
    void* addr = NULL;

    uint32_t offset = mymem_malloc(memx, size);
    if (offset != 0xffffffff) {
        addr = (void*)((uintptr_t)malloc_dev.mempool[memx] + offset);
#if CONFIG_MEMORY_POOL_DEBUG
        add_a_tracer_record(memx, (uint16_t)size, addr, get_filename(file_name),
                            func_line);
#endif
    }

    mutex_unlock(memx);
    return addr;
}

#if 0
void* myrealloc(uint8_t memx, void* ptr, uint32_t size)
{
    mutex_lock(memx);
    uint32_t offset = mymem_malloc(memx, size);
    if (offset != 0xffffffff) {
        mymemcpy((void*)((uintptr_t)malloc_dev.mempool[memx] + offset), ptr,
                 size);
        MYFREE(ptr);
        ptr = NULL;
        mutex_unlock(memx);

        return (void*)((uintptr_t)malloc_dev.mempool[memx] + offset);
    }
    mutex_unlock(memx);
    return NULL;
}
#endif
