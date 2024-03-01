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

#ifndef _MALLOC_H_
#define _MALLOC_H_

#include <stdbool.h>
#include <stdint.h>

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif /* UNUSED */

#define SRAMIN   0x00
#define SRAMEX   0x01
#define SRAMCCM  0x02
#define SRAMEX1  0x03
#define SRAMEX2  0x04
#define SRAMBANK (SRAMEX2 + 1)

#define INSRAM        // __attribute__((at(0x30000000 + 0x00000000)));
#define EXTRAM        // __attribute__((at(0x40000000 + 0x00000000)));
#define CCMRAM        // __attribute__((at(0x50000000 + 0x00000000)));

#define ALIGN_SIZE    // __align(4)

#define MEM1_BLOCK_SIZE   32
#define MEM1_POOL_SIZE    100 * 1024
#define MEM1_TABLE_SIZE   MEM1_POOL_SIZE / MEM1_BLOCK_SIZE

#define MEM2_BLOCK_SIZE   32
#define MEM2_POOL_SIZE    100 * 1024
#define MEM2_TABLE_SIZE   MEM2_POOL_SIZE / MEM2_BLOCK_SIZE

#define MEM3_BLOCK_SIZE   32
#define MEM3_POOL_SIZE    32
#define MEM3_TABLE_SIZE   MEM3_POOL_SIZE / MEM3_BLOCK_SIZE

#define MEM4_BLOCK_SIZE   32
#define MEM4_POOL_SIZE    50 * 1024
#define MEM4_TABLE_SIZE   MEM4_POOL_SIZE / MEM4_BLOCK_SIZE

#define MEM5_BLOCK_SIZE   32
#define MEM5_POOL_SIZE    50 * 1024
#define MEM5_TABLE_SIZE   MEM5_POOL_SIZE / MEM5_BLOCK_SIZE

#define MYMALLOC(memx, size) mymalloc((memx), (size), __FILE__, __LINE__)
#define MYFREE(ptr)          myfree((ptr), __FILE__, __LINE__)

void mymem_init(uint8_t memx);

void* mymalloc(uint8_t memx, uint32_t size, char* file_name, uint32_t func_line);

void myfree(void* ptr, char* file_name, uint32_t func_line);

void mymemset(void* src, uint8_t c, uint32_t count);

void mymemcpy(void* des, void* src, uint32_t size);

/* just for debug, print memory use information */
uint8_t mem_perused(uint8_t memx);

#if 0
void* myrealloc(uint8_t memx, void* ptr, uint32_t size);
#endif

#endif /* _MALLOC_H_ */
