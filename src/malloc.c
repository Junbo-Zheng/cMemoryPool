#include "malloc.h"

#include <stddef.h>
#include <stdio.h>

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

#if CONFIG_MEMORY_POOL_DEBUG
#define TRACER_NODE_NUM   (1000)
#define TRACER_MEMX_NUM   (SRAMBANK)
#define TRACER_INDEX_NUM  (256)
#define TRACER_REFREE_NUM (100)

#if TRACER_MEMX_NUM > SRAMBANK
#error "TRACER_MEMX_NUM is error"
#endif

#if TRACER_INDEX_NUM > 256
#error "TRACER_INDEX_NUM is error"
#endif

#if TRACER_REFREE_NUM > 65535
#error "TRACER_REFREE_NUM is error"
#endif

// bit flag mask
#define BIT_MASK_FREEEMPTY (0x0001)  // 可用记录条数为0
#define BIT_MASK_OVERFLOW  (0x0002)  // tracer资源溢出过

typedef struct _tracer_node {
    struct _tracer_node* p_next;
    char*    file_name;
    uint32_t func_line;
    void*    malloc_ptr;
    uint16_t mem_sz;
    uint8_t  memx;
} __attribute__((packed)) tracer_node_t, *p_tracer_node_t;

typedef struct _tracer_node_hd {
    p_tracer_node_t p_next;
    p_tracer_node_t p_tail;
    uint16_t        node_cnt;
} __attribute__((packed)) tracer_node_hd_t, *p_tracer_node_hd_t;

typedef struct _pos_info {
    char*    file_name;
    uint32_t func_line;
} __attribute__((packed)) pos_info_t, *p_pos_info_t;

typedef struct _repeat_statistic {
    pos_info_t pos_info;
    uint16_t   cnt;
} __attribute__((packed)) repeat_statistic_t, *p_repeat_statistic_t;

typedef struct _refree_statistic {
    pos_info_t pos_info[TRACER_REFREE_NUM];
    uint16_t   cnt;
} __attribute__((packed)) refree_statistic_t, *p_refree_statistic_t;

typedef struct _tracer_list {
    tracer_node_hd_t   used_node_hd;
    tracer_node_hd_t   free_node_hd;
    int32_t            malloc_free_cnt;
    uint32_t           mem_statistic[TRACER_MEMX_NUM];
    repeat_statistic_t repeat_statistic[TRACER_INDEX_NUM];
    refree_statistic_t refree_statistic;
    uint16_t           flag;
} __attribute__((packed)) tracer_list_t, *p_tracer_list_t;

EXTRAM tracer_node_t tracer_node[TRACER_NODE_NUM];
EXTRAM tracer_list_t tracer_list;

// OS_MUTEX tracer_mtx;

void pend_mtx(void)
{
    // OS_ERR os_err;
    // if (OSRunning != OS_STATE_OS_RUNNING) return;

    // OSMutexPend(&tracer_mtx, 0, OS_OPT_PEND_BLOCKING, 0, &os_err);
}

void post_mtx(void)
{
    // OS_ERR os_err;
    // if (OSRunning != OS_STATE_OS_RUNNING) return;

    // OSMutexPost(&tracer_mtx, OS_OPT_POST_NO_SCHED, &os_err);
}

//初始化
static void init_tracer(void)
{
    //清空list
    memset((void*)&tracer_list, 0, sizeof(tracer_list_t));

    //初始化free链表
    for (uint32_t i = 0; i < (TRACER_NODE_NUM - 1); i++) {
        tracer_node[i].p_next     = &tracer_node[i + 1];
        tracer_node[i].file_name  = NULL;
        tracer_node[i].func_line  = 0;
        tracer_node[i].malloc_ptr = 0;
        tracer_node[i].mem_sz     = 0;
        tracer_node[i].memx       = 0;
    }
    tracer_node[TRACER_NODE_NUM - 1].p_next     = NULL;
    tracer_node[TRACER_NODE_NUM - 1].file_name  = NULL;
    tracer_node[TRACER_NODE_NUM - 1].func_line  = 0;
    tracer_node[TRACER_NODE_NUM - 1].malloc_ptr = 0;
    tracer_node[TRACER_NODE_NUM - 1].mem_sz     = 0;
    tracer_node[TRACER_NODE_NUM - 1].memx       = 0;

    tracer_list.free_node_hd.p_next   = tracer_node;
    tracer_list.free_node_hd.p_tail   = tracer_node + TRACER_NODE_NUM - 1;
    tracer_list.free_node_hd.node_cnt = TRACER_NODE_NUM;
    tracer_list.used_node_hd.p_next   = NULL;
    tracer_list.used_node_hd.p_tail   = NULL;
    tracer_list.used_node_hd.node_cnt = 0;

    OS_ERR os_err;
    OSMutexCreate(&tracer_mtx, (CPU_CHAR*)"tracer_mtx", &os_err);
}

//从链表头上申请一个节点
p_tracer_node_t remove_node(p_tracer_node_hd_t p_node_hd)
{
    if (p_node_hd == NULL) {
        return NULL;
    }

    p_tracer_node_t p_node;
    if (p_node_hd->p_next) {
        p_node            = p_node_hd->p_next;
        p_node_hd->p_next = p_node->p_next;
        if (p_node_hd->p_next == NULL) {
            p_node_hd->p_tail = NULL;
        }

        p_node->p_next = NULL;

        if (p_node_hd->node_cnt) {
            p_node_hd->node_cnt--;
        }

        return p_node;
    }

    return NULL;
}

//插入链表尾部
bool insert_node(p_tracer_node_hd_t p_node_hd, p_tracer_node_t p_node)
{
    if ((p_node_hd == NULL) || (p_node == NULL)) {
        return false;
    }

    if (p_node_hd->p_next) {
        p_node_hd->p_tail->p_next = p_node;
    } else {
        p_node_hd->p_next = p_node;
    }
    p_node_hd->p_tail = p_node;

    p_node->p_next = NULL;
    p_node_hd->node_cnt++;
    return true;
}

//查找并移除一个节点
p_tracer_node_t find_and_remove_node(p_tracer_node_hd_t p_node_hd,
                                     void* malloc_ptr)
{
    if (p_node_hd == NULL) {
        return NULL;
    }

    p_tracer_node_t p_node        = p_node_hd->p_next;
    p_tracer_node_t p_node_former = NULL;
    while (p_node) {
        if (p_node->malloc_ptr == malloc_ptr) {
            if (p_node_former) {
                p_node_former->p_next = p_node->p_next;
            } else {
                p_node_hd->p_next = p_node->p_next;
            }

            if (p_node_hd->p_tail == p_node) {
                p_node_hd->p_tail = p_node_former;
            }

            p_node->p_next = NULL;

            if (p_node_hd->node_cnt) {
                p_node_hd->node_cnt--;
            }

            return p_node;
        } else {
            p_node_former = p_node;
            p_node        = p_node->p_next;
        }
    }

    return NULL;
}

//添加一个tracer记录
bool add_a_tracer_record(uint8_t memx, uint16_t mem_sz, void* malloc_ptr,
                         char* file_name, uint32_t func_line)
{
    pend_mtx();

    tracer_list.malloc_free_cnt++;

    if (memx > SRAMBANK) {
        post_mtx();
        return false;
    }

    p_tracer_node_t p_node = remove_node(&tracer_list.free_node_hd);

    if (p_node == NULL) {
        tracer_list.flag |= BIT_MASK_FREEEMPTY;
        tracer_list.flag |= BIT_MASK_OVERFLOW;
        post_mtx();
        return false;
    }

    p_node->malloc_ptr = malloc_ptr;
    p_node->file_name  = file_name;
    p_node->func_line  = func_line;
    p_node->memx       = memx;
    p_node->mem_sz     = mem_sz;

    bool res = insert_node(&tracer_list.used_node_hd, p_node);

    post_mtx();
    return res;
}

//删除一个tracer记录
bool del_a_tracer_record(void* malloc_ptr, char* file_name, uint32_t func_line)
{
    pend_mtx();

    tracer_list.malloc_free_cnt--;

    p_tracer_node_t p_node
         =  find_and_remove_node(&tracer_list.used_node_hd, malloc_ptr);

    if (p_node == NULL) {
        //重复释放(没找到节点且used链表没有溢出且used链表又不是空)
        if (!(tracer_list.flag & BIT_MASK_OVERFLOW)
            && (tracer_list.used_node_hd.node_cnt)) {
            if (tracer_list.refree_statistic.cnt < TRACER_REFREE_NUM) {
                tracer_list.refree_statistic
                    .pos_info[tracer_list.refree_statistic.cnt]
                    .file_name
                     =  file_name;
                tracer_list.refree_statistic
                    .pos_info[tracer_list.refree_statistic.cnt]
                    .func_line
                     =  func_line;
                tracer_list.refree_statistic.cnt++;
            }
        }
        post_mtx();
        return false;
    }

    if (insert_node(&tracer_list.free_node_hd, p_node) == true) {
        tracer_list.flag &= (~BIT_MASK_FREEEMPTY);
        post_mtx();
        return true;
    } else {
        post_mtx();
        return false;
    }
}

//从绝对路径中获取文件名
char* get_filename(char* path)
{
    char* ptr = path;

    // find tail
    while (*ptr) {
        ptr++;
    }

    // find the last '\\'
    while (*ptr != '\\') {
        *ptr--;

        if (ptr == path) {
            return ptr;
        }
    }

    ptr++;
    return ptr;
}

//获取信息
int32_t get_tracer_malloc_free_cnt(void) { return tracer_list.malloc_free_cnt; }

bool printf_tracer_info(void)
{
    static uint8_t print_buf[1024];

    pend_mtx();
    //重要，信号量保护段内不能有日志输出，否则会造成获取信号量嵌套导致死机!!!!!!!!

    int32_t  malloc_free_cnt = tracer_list.malloc_free_cnt;
    uint8_t  flag            = tracer_list.flag;
    uint16_t free_node_cnt   = tracer_list.free_node_hd.node_cnt;
    uint16_t used_node_cnt   = tracer_list.used_node_hd.node_cnt;

    memset((void*)(tracer_list.mem_statistic), 0,
           sizeof(uint32_t) * TRACER_MEMX_NUM);
    memset((void*)(tracer_list.repeat_statistic), 0,
           sizeof(repeat_statistic_t) * TRACER_INDEX_NUM);

    p_tracer_node_t p_node = tracer_list.used_node_hd.p_next;

    while (p_node) {
        tracer_list.mem_statistic[p_node->memx] += p_node->mem_sz;

        int i = 0;
        while (i < TRACER_INDEX_NUM) {
            if (tracer_list.repeat_statistic[i].pos_info.file_name) {
                if ((tracer_list.repeat_statistic[i].pos_info.file_name
                     == p_node->file_name)
                    && (tracer_list.repeat_statistic[i].pos_info.func_line
                        == p_node->func_line)) {
                    //找到一个相同的refree节点，统计值加一
                    tracer_list.repeat_statistic[i].cnt++;
                    break;
                }
            } else {
                //当前元素为空，说明之前没有相同的refree节点，则新增一个
                tracer_list.repeat_statistic[i].pos_info.file_name
                     =  p_node->file_name;
                tracer_list.repeat_statistic[i].pos_info.func_line
                                                        =  p_node->func_line;
                    tracer_list.repeat_statistic[i].cnt = 1;
                break;
            }

            i++;
        }

        p_node = p_node->p_next;
    }

    post_mtx();

    PRINT_TEST("tracer_list.malloc_free_cnt = %d, tracer_list.flag = 0x%04x, "
               "free node cnt = %u, used node cnt = %u",
               malloc_free_cnt, flag, free_node_cnt, used_node_cnt);

    PRINT_TEST("SRAMIN:%u\tSRAMEX:%u\tSRAMCCM:%u\tSRAMEX1:%u\tSRAMEX2:%u\t",
               tracer_list.mem_statistic[0], tracer_list.mem_statistic[1],
               tracer_list.mem_statistic[2], tracer_list.mem_statistic[3],
               tracer_list.mem_statistic[4]);

    uint8_t* p_buf  = print_buf;
             p_buf += sprintf((R_Str)p_buf, "malloc : ");
    for (uint16_t i = 0, j = 0; i < TRACER_INDEX_NUM; i++) {
        if (tracer_list.repeat_statistic[i].cnt) {
            j++;
            p_buf += sprintf((R_Str)p_buf, "%s(%u)=%u\t",
                             tracer_list.repeat_statistic[i].pos_info.file_name,
                             tracer_list.repeat_statistic[i].pos_info.func_line,
                             tracer_list.repeat_statistic[i].cnt);
        }

        if (j == 4) {
            j     = 0;
            p_buf = print_buf;
            PRINT_TEST("%s", p_buf);
            p_buf += sprintf((R_Str)p_buf, "malloc : ");
        } else {
            if ((i == (TRACER_INDEX_NUM - 1)) && j) {
                j     = 0;
                p_buf = print_buf;
                PRINT_TEST("%s", p_buf);
                p_buf += sprintf((R_Str)p_buf, "malloc : ");
            }
        }
    }

    if (tracer_list.refree_statistic.cnt) {
        PRINT_TEST("refree pointer,total %u:",
                   tracer_list.refree_statistic.cnt);
    }

    p_buf  = print_buf;
    p_buf += sprintf((R_Str)p_buf, "refree: ");
    for (uint16_t i = 0, j = 0; i < TRACER_REFREE_NUM; i++) {
        if (tracer_list.refree_statistic.pos_info[i].file_name) {
            j++;
            p_buf
                += sprintf((R_Str)p_buf, "%s(%u)\t",
                           tracer_list.refree_statistic.pos_info[i].file_name,
                           tracer_list.refree_statistic.pos_info[i].func_line);
        }

        if (j == 4) {
            j     = 0;
            p_buf = print_buf;
            PRINT_TEST("%s", p_buf);
            p_buf += sprintf((R_Str)p_buf, "refree: ");
        } else {
            if ((i == (TRACER_REFREE_NUM - 1)) && j) {
                j     = 0;
                p_buf = print_buf;
                PRINT_TEST("%s", p_buf);
                p_buf += sprintf((R_Str)p_buf, "refree: ");
            }
        }
    }

    return true;
}

// OS_MUTEX MallocMutex[SRAMBANK];

#endif

static void mutex_creat(uint8_t memx)
{

}

static void mutex_pend(uint8_t memx)
{

}

static void mutex_post(uint8_t memx)
{

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

uint32_t mymem_malloc(uint8_t memx, uint32_t size)
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

//释放内存(内部调用)
// memx:所属内存块
// offset:内存地址偏移
//返回值:0,释放成功;1,释放失败;
uint8_t mymem_free(uint8_t memx, uint32_t offset)
{
    if (!malloc_dev.memready[memx]) {
        malloc_dev.init(memx);
        return 1;
    }

    if (offset < mempoolsize[memx]) {
        int index = offset / memblocksize[memx]; //偏移所在内存块号码
        int nmemb = malloc_dev.memtable[memx][index]; //内存块数量
        for (uint16_t i = 0; i < nmemb; i++) {
            malloc_dev.memtable[memx][index + i] = 0;
        }
        return 0;
    }
    return 2; //偏移超区了
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
        mutex_pend(memx);

        uint32_t offset = (uintptr_t)ptr - (uintptr_t)malloc_dev.mempool[memx];
        mymem_free(memx, offset);
#if CONFIG_MEMORY_POOL_DEBUG
        del_a_tracer_record(ptr, get_filename(file_name), func_line);
#endif
        mutex_post(memx);
    }

    ptr = NULL;
}

void* mymalloc(uint8_t memx, uint32_t size, char* file_name, uint32_t func_line)
{
    mutex_pend(memx);
    void* addr = NULL;

    uint32_t offset = mymem_malloc(memx, size);
    if (offset != 0xffffffff) {
        addr = (void*)((uintptr_t)malloc_dev.mempool[memx] + offset);
#if CONFIG_MEMORY_POOL_DEBUG
        add_a_tracer_record(memx, (uint16_t)size, addr, get_filename(file_name),
                            func_line);
#endif
    }

    mutex_post(memx);
    return addr;
}

#if 0
void* myrealloc(uint8_t memx, void* ptr, uint32_t size)
{
    mutex_pend(memx);
    uint32_t offset = mymem_malloc(memx, size);
    if (offset != 0xffffffff) {
        mymemcpy((void*)((uintptr_t)malloc_dev.mempool[memx] + offset), ptr,
                 size);
        MYFREE(ptr);
        ptr = NULL;
        mutex_post(memx);

        return (void*)((uintptr_t)malloc_dev.mempool[memx] + offset);
    }
    mutex_post(memx);
    return NULL;
}
#endif
