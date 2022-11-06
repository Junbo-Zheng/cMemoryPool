#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdbool.h>
#include <stdint.h>

#ifndef __PACKED
#define __PACKED __attribute__((packed))
#endif

void init_tracer(void);

bool del_a_tracer_record(void* malloc_ptr, char* file_name, uint32_t func_line);

char* get_filename(char* path);

bool add_a_tracer_record(uint8_t memx, uint16_t mem_sz, void* malloc_ptr,
                         char* file_name, uint32_t func_line);

#endif /* _DEBUG_H_ */
