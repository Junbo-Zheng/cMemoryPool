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
