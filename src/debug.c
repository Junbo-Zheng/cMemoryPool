/*
 * Copyright (C) 2022 Junbo Zheng. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "debug.h"

#include <stdio.h>
#include <string.h>

#if __linux__
#include <pthread.h>
#endif

#include "malloc.h"

#define TRACER_NODE_NUM   (256)
#define TRACER_MEMX_NUM   (SRAMBANK)
#define TRACER_REPEAT_NUM (256)
#define TRACER_REFREE_NUM (100)

#if TRACER_MEMX_NUM > SRAMBANK
#error "TRACER_MEMX_NUM error"
#endif

#if TRACER_REPEAT_NUM > 512
#error "TRACER_REPEAT_NUM error"
#endif

#if TRACER_REFREE_NUM > 512
#error "TRACER_REFREE_NUM error"
#endif

#ifndef __PACKED
#define __PACKED __attribute__((packed))
#endif

#define BIT_MASK_EMPTY     (0x01)  /* used node is empty */
#define BIT_MASK_OVERFLOW  (0x02)

typedef struct tracer_node {
    struct tracer_node* p_next;
    char*    file_name;
    uint32_t func_line;
    void*    malloc_ptr;
    uint32_t mem_sz;
    uint8_t  memx;
} __PACKED tracer_node_t;

typedef struct {
    tracer_node_t* p_next;
    tracer_node_t* p_tail;
    uint16_t       count;
} __PACKED tracer_node_data_t;

typedef struct {
    char*    file_name;
    uint32_t func_line;
} __PACKED pos_info_t;

typedef struct {
    pos_info_t pos_info;
    uint16_t   count;
} __PACKED repeat_statistic_t;

typedef struct {
    pos_info_t pos_info[TRACER_REFREE_NUM];
    uint16_t   count;
} __PACKED refree_statistic_t;

typedef struct {
    bool init;
    tracer_node_data_t used_node;    /* used node from unused node */
    tracer_node_data_t unused_node;  /* unused node, not free memory node */
    int32_t            malloc_free_cnt;
    uint32_t           mem_statistic[TRACER_MEMX_NUM];
    repeat_statistic_t repeat_statistic[TRACER_REPEAT_NUM];
    refree_statistic_t refree_statistic;
    uint16_t           flag;
} __PACKED tracer_list_t;

static EXTRAM tracer_node_t tracer_node[TRACER_NODE_NUM] = { 0 };
static EXTRAM tracer_list_t tracer_list = { 0 };

#if __linux__
static pthread_mutex_t mutex = { 0 };
#endif

static void debug_mutex_init(void)
{
#if __linux__
    pthread_mutex_init(&mutex, NULL);
#endif
}

static void debug_mutex_lock(void)
{
#if __linux__
    pthread_mutex_lock(&mutex);
#endif
}

static void debug_mutex_unlock(void)
{
#if __linux__
    pthread_mutex_unlock(&mutex);
#endif
}

static char* get_filename(const char* path)
{
#if __linux__
    char* ptr = strrchr(path, '/');
#else
    char* ptr = strrchr(path, '\\');
#endif
    return ptr + 1;
}

void memory_pool_debug_init(void)
{
    if (tracer_list.init == true) {
        return;
    }

    memset((void*)&tracer_list, 0, sizeof(tracer_list_t));

    /* init free list */
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

    tracer_list.unused_node.p_next = tracer_node;
    tracer_list.unused_node.p_tail = tracer_node + TRACER_NODE_NUM - 1;
    tracer_list.unused_node.count  = TRACER_NODE_NUM;

    tracer_list.used_node.p_next = NULL;
    tracer_list.used_node.p_tail = NULL;
    tracer_list.used_node.count  = 0;

    debug_mutex_init();

    tracer_list.init = true;
}

static tracer_node_t* alloc_unused_node(tracer_node_data_t* p_node_data)
{
    if (p_node_data == NULL) {
        return NULL;
    }

    tracer_node_t* p_node = NULL;
    if (p_node_data->p_next) {
        p_node = p_node_data->p_next;
        p_node_data->p_next = p_node->p_next;
        if (p_node_data->p_next == NULL) {
            p_node_data->p_tail = NULL;
        }

        p_node->p_next = NULL;

        if (p_node_data->count) {
            p_node_data->count--;
        }

        return p_node;
    }

    return NULL;
}

static bool insert_node(tracer_node_data_t* p_node_data, tracer_node_t* p_node)
{
    if ((p_node_data == NULL) || (p_node == NULL)) {
        return false;
    }

    if (p_node_data->p_next) {
        p_node_data->p_tail->p_next = p_node;
    } else {
        p_node_data->p_next = p_node;
    }
    p_node_data->p_tail = p_node;

    p_node->p_next = NULL;
    p_node_data->count++;
    return true;
}

static tracer_node_t* find_and_remove_node(tracer_node_data_t* p_node_data,
                                           void* malloc_ptr)
{
    if (p_node_data == NULL) {
        return NULL;
    }

    tracer_node_t* p_node = p_node_data->p_next;
    tracer_node_t* p_node_former = NULL;
    while (p_node) {
        if (p_node->malloc_ptr == malloc_ptr) {
            if (p_node_former) {
                p_node_former->p_next = p_node->p_next;
            } else {
                p_node_data->p_next = p_node->p_next;
            }

            if (p_node_data->p_tail == p_node) {
                p_node_data->p_tail = p_node_former;
            }

            p_node->p_next = NULL;

            if (p_node_data->count) {
                p_node_data->count--;
            }

            return p_node;
        }

        p_node_former = p_node;
        p_node = p_node->p_next;
    }

    return NULL;
}

bool memory_pool_debug_add(uint8_t memx, uint32_t mem_sz, void* malloc_ptr,
                           char* file_name, uint32_t func_line)
{
    debug_mutex_lock();

    tracer_list.malloc_free_cnt++;

    if (memx > SRAMBANK) {
        debug_mutex_unlock();
        return false;
    }

    tracer_node_t* p_node = alloc_unused_node(&tracer_list.unused_node);
    if (p_node == NULL) {
        tracer_list.flag |= BIT_MASK_EMPTY;
        tracer_list.flag |= BIT_MASK_OVERFLOW;
        debug_mutex_unlock();
        return false;
    }

    p_node->malloc_ptr = malloc_ptr;
    p_node->file_name  = get_filename(file_name);
    p_node->func_line  = func_line;
    p_node->memx       = memx;
    p_node->mem_sz     = mem_sz;

    bool ret = insert_node(&tracer_list.used_node, p_node);

    debug_mutex_unlock();
    return ret;
}

bool memory_pool_debug_del(void* malloc_ptr, char* file_name,
                           uint32_t func_line)
{
    debug_mutex_lock();

    tracer_list.malloc_free_cnt--;

    tracer_node_t* p_node
        = find_and_remove_node(&tracer_list.used_node, malloc_ptr);

    if (p_node == NULL) {
        /* refree the same address, and the used list is not BIT_MASK_OVERFLOW
         * and is not BIT_MASK_EMPTY
         */
        if (!(tracer_list.flag & BIT_MASK_OVERFLOW)
            && (tracer_list.used_node.count)) {
            if (tracer_list.refree_statistic.count < TRACER_REFREE_NUM) {
                tracer_list.refree_statistic
                    .pos_info[tracer_list.refree_statistic.count]
                    .file_name
                    = get_filename(file_name);
                tracer_list.refree_statistic
                    .pos_info[tracer_list.refree_statistic.count]
                    .func_line
                    = func_line;
                tracer_list.refree_statistic.count++;
            }
        }
        debug_mutex_unlock();
        return false;
    }

    /* revert malloc ptr to unused node */
    if (insert_node(&tracer_list.unused_node, p_node)) {
        /* clear BIT_MASK_EMPTY bit map */
        tracer_list.flag &= (~BIT_MASK_EMPTY);
        debug_mutex_unlock();
        return true;
    }

    debug_mutex_unlock();
    return false;
}

int32_t memory_pool_debug_malloc_free_count(void)
{
    int32_t count = 0;
    debug_mutex_lock();
    count = tracer_list.malloc_free_cnt;
    debug_mutex_unlock();
    return count;
}

void memory_pool_debug_trace(void)
{
    uint8_t print_buf[1024] = { 0 };

    debug_mutex_lock();

    int32_t  malloc_free_cnt = tracer_list.malloc_free_cnt;
    uint8_t  flag            = tracer_list.flag;
    uint16_t unused_node_cnt = tracer_list.unused_node.count;
    uint16_t used_node_cnt   = tracer_list.used_node.count;

    memset((void*)(tracer_list.mem_statistic), 0,
           sizeof(uint32_t) * TRACER_MEMX_NUM);
    memset((void*)(tracer_list.repeat_statistic), 0,
           sizeof(repeat_statistic_t) * TRACER_REPEAT_NUM);

    tracer_node_t* p_node = tracer_list.used_node.p_next;

    while (p_node) {
        tracer_list.mem_statistic[p_node->memx] += p_node->mem_sz;

        uint16_t i = 0;
        while (i < TRACER_REPEAT_NUM) {
            if (tracer_list.repeat_statistic[i].pos_info.file_name) {
                if ((tracer_list.repeat_statistic[i].pos_info.file_name
                     == p_node->file_name)
                    && (tracer_list.repeat_statistic[i].pos_info.func_line
                        == p_node->func_line)) {
                    tracer_list.repeat_statistic[i].count++;
                    break;
                }
            } else {
                tracer_list.repeat_statistic[i].pos_info.file_name
                    = p_node->file_name;
                tracer_list.repeat_statistic[i].pos_info.func_line
                    = p_node->func_line;
                tracer_list.repeat_statistic[i].count = 1;
                break;
            }

            i++;
        }

        p_node = p_node->p_next;
    }

    debug_mutex_unlock();

    printf("tracer_list.malloc_free_cnt = %d\n", malloc_free_cnt);
    printf("tracer_list.unused node cnt = %u\n", unused_node_cnt);
    printf("tracer_list.used   node cnt = %u\n", used_node_cnt);
    printf("tracer_list.flag            = 0x%02x\n", flag);

    printf("SRAMIN  : %u\n", tracer_list.mem_statistic[0]);
    printf("SRAMEX  : %u\n", tracer_list.mem_statistic[1]);
    printf("SRAMCCM : %u\n", tracer_list.mem_statistic[2]);
    printf("SRAMEX1 : %u\n", tracer_list.mem_statistic[3]);
    printf("SRAMEX2 : %u\n", tracer_list.mem_statistic[4]);

    uint8_t* p_buf = print_buf;
    p_buf += sprintf((char*)p_buf, "malloc : ");
    for (uint16_t i = 0, j = 0; i < TRACER_REPEAT_NUM; i++) {
        if (tracer_list.repeat_statistic[i].count) {
            j++;
            p_buf += sprintf((char*)p_buf, "%s(%u) = %u\t",
                             tracer_list.repeat_statistic[i].pos_info.file_name,
                             tracer_list.repeat_statistic[i].pos_info.func_line,
                             tracer_list.repeat_statistic[i].count);
        }

        if (j == 4) {
            j = 0;
            p_buf = print_buf;
            printf("%s", p_buf);
            p_buf += sprintf((char*)p_buf, "malloc : ");
        } else {
            if ((i == (TRACER_REPEAT_NUM - 1)) && j) {
                j = 0;
                p_buf = print_buf;
                printf("%s", p_buf);
                p_buf += sprintf((char*)p_buf, "malloc : ");
            }
        }
    }

    printf("\n");

    if (tracer_list.refree_statistic.count) {
        printf("refree total count: %u\n", tracer_list.refree_statistic.count);
    }

    memset(print_buf, 0, sizeof(print_buf));
    p_buf = print_buf;
    p_buf += sprintf((char*)p_buf, "refree: ");
    for (uint16_t i = 0, j = 0; i < TRACER_REFREE_NUM; i++) {
        if (tracer_list.refree_statistic.pos_info[i].file_name) {
            j++;
            p_buf
                += sprintf((char*)p_buf, "%s(%u)\t",
                           tracer_list.refree_statistic.pos_info[i].file_name,
                           tracer_list.refree_statistic.pos_info[i].func_line);
        }

        if (j == 4) {
            j = 0;
            p_buf = print_buf;
            printf("%s", p_buf);
            p_buf += sprintf((char*)p_buf, "refree: ");
        } else {
            if ((i == (TRACER_REFREE_NUM - 1)) && j) {
                j = 0;
                p_buf = print_buf;
                printf("%s", p_buf);
                p_buf += sprintf((char*)p_buf, "refree: ");
            }
        }
    }
    printf("\n");
}
