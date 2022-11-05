
#include "debug.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <pthread.h>

#include "malloc.h"

#define TRACER_NODE_NUM   (1000)
#define TRACER_MEMX_NUM   (SRAMBANK)
#define TRACER_INDEX_NUM  (256)
#define TRACER_REFREE_NUM (100)

#if TRACER_MEMX_NUM > SRAMBANK
#error "TRACER_MEMX_NUM error"
#endif

#if TRACER_INDEX_NUM > 256
#error "TRACER_INDEX_NUM error"
#endif

#if TRACER_REFREE_NUM > 65535
#error "TRACER_REFREE_NUM error"
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
} __PACKED tracer_node_t, *p_tracer_node_t;

typedef struct _tracer_node_hd {
    p_tracer_node_t p_next;
    p_tracer_node_t p_tail;
    uint16_t        node_cnt;
} __PACKED tracer_node_hd_t, *p_tracer_node_hd_t;

typedef struct _pos_info {
    char*    file_name;
    uint32_t func_line;
} __PACKED pos_info_t, *p_pos_info_t;

typedef struct _repeat_statistic {
    pos_info_t pos_info;
    uint16_t   cnt;
} __PACKED repeat_statistic_t, *p_repeat_statistic_t;

typedef struct _refree_statistic {
    pos_info_t pos_info[TRACER_REFREE_NUM];
    uint16_t   cnt;
} __PACKED refree_statistic_t, *p_refree_statistic_t;

typedef struct _tracer_list {
    tracer_node_hd_t   used_node_hd;
    tracer_node_hd_t   free_node_hd;
    int32_t            malloc_free_cnt;
    uint32_t           mem_statistic[TRACER_MEMX_NUM];
    repeat_statistic_t repeat_statistic[TRACER_INDEX_NUM];
    refree_statistic_t refree_statistic;
    uint16_t           flag;
} __PACKED tracer_list_t, *p_tracer_list_t;

EXTRAM tracer_node_t tracer_node[TRACER_NODE_NUM];
EXTRAM tracer_list_t tracer_list;

static pthread_mutex_t mutex = { 0 };

static void debug_mutex_init(void)
{
    pthread_mutex_init(&mutex, NULL);
}

static void debug_mutex_lock(void)
{
    pthread_mutex_lock(&mutex);
}

static void debug_mutex_unlock(void)
{
    pthread_mutex_unlock(&mutex);
}

static void init_tracer(void)
{
    //清空list
    memset((void*)&tracer_list, 0, sizeof(tracer_list_t));

    //初始化free链表
    for (uint16_t i = 0; i < (TRACER_NODE_NUM - 1); i++) {
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

    debug_mutex_init();
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
        }

        p_node_former = p_node;
        p_node        = p_node->p_next;
    }

    return NULL;
}

//添加一个tracer记录
bool add_a_tracer_record(uint8_t memx, uint16_t mem_sz, void* malloc_ptr,
                         char* file_name, uint32_t func_line)
{
    debug_mutex_lock();

    tracer_list.malloc_free_cnt++;

    if (memx > SRAMBANK) {
        debug_mutex_unlock();
        return false;
    }

    p_tracer_node_t p_node = remove_node(&tracer_list.free_node_hd);

    if (p_node == NULL) {
        tracer_list.flag |= BIT_MASK_FREEEMPTY;
        tracer_list.flag |= BIT_MASK_OVERFLOW;
        debug_mutex_unlock();
        return false;
    }

    p_node->malloc_ptr = malloc_ptr;
    p_node->file_name  = file_name;
    p_node->func_line  = func_line;
    p_node->memx       = memx;
    p_node->mem_sz     = mem_sz;

    bool res = insert_node(&tracer_list.used_node_hd, p_node);

    debug_mutex_unlock();
    return res;
}

//删除一个tracer记录
bool del_a_tracer_record(void* malloc_ptr, char* file_name, uint32_t func_line)
{
    debug_mutex_lock();

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
        debug_mutex_unlock();
        return false;
    }

    if (insert_node(&tracer_list.free_node_hd, p_node) == true) {
        tracer_list.flag &= (~BIT_MASK_FREEEMPTY);
        debug_mutex_unlock();
        return true;
    }

    debug_mutex_unlock();
    return false;
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

    debug_mutex_lock();
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

    debug_mutex_unlock();

    printf("tracer_list.malloc_free_cnt = %d, tracer_list.flag = 0x%04x, "
           "free node cnt = %u, used node cnt = %u",
           malloc_free_cnt, flag, free_node_cnt, used_node_cnt);

    printf("SRAMIN:%u\tSRAMEX:%u\tSRAMCCM:%u\tSRAMEX1:%u\tSRAMEX2:%u\t",
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
            printf("%s", p_buf);
            p_buf += sprintf((R_Str)p_buf, "malloc : ");
        } else {
            if ((i == (TRACER_INDEX_NUM - 1)) && j) {
                j     = 0;
                p_buf = print_buf;
                printf("%s", p_buf);
                p_buf += sprintf((R_Str)p_buf, "malloc : ");
            }
        }
    }

    if (tracer_list.refree_statistic.cnt) {
        printf("refree pointer,total %u:", tracer_list.refree_statistic.cnt);
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
            printf("%s", p_buf);
            p_buf += sprintf((R_Str)p_buf, "refree: ");
        } else {
            if ((i == (TRACER_REFREE_NUM - 1)) && j) {
                j     = 0;
                p_buf = print_buf;
                printf("%s", p_buf);
                p_buf += sprintf((R_Str)p_buf, "refree: ");
            }
        }
    }

    return true;
}
