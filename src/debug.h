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

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdbool.h>
#include <stdint.h>

void memory_pool_debug_init(void);

bool memory_pool_debug_add(uint8_t memx, uint32_t mem_sz, void* malloc_ptr,
                           char* file_name, uint32_t func_line);

bool memory_pool_debug_del(void* malloc_ptr, char* file_name,
                           uint32_t func_line);

int32_t memory_pool_debug_malloc_free_count(void);

void memory_pool_debug_trace(void);

#endif /* _DEBUG_H_ */
