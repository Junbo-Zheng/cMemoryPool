#include "./src/malloc.h"

#if CONFIG_MEMORY_POOL_DEBUG
#include "./src/debug.h"
#endif

#include <stdio.h>
#include <stdlib.h>

static void exit_function(void)
{
    printf("exit funxtion run start\n");

#if CONFIG_MEMORY_POOL_DEBUG
    memory_pool_debug_trace();
    printf("malloc free count: %d\n", memory_pool_debug_malloc_free_count());
#endif

    printf("exit funxtion run end\n");
}

int main(void)
{
    mymem_init(SRAMIN);
    mymem_init(SRAMEX);
    mymem_init(SRAMCCM);
    mymem_init(SRAMEX1);
    mymem_init(SRAMEX2);

    uint8_t* ptr = MYMALLOC(SRAMCCM, 12);
    if (ptr) {
        printf("ptr malloc ok, addr: %p\n", ptr);
        printf("ptr memory use info: %d%%\n", mem_perused(SRAMCCM));
    } else {
        printf("ptr malloc fail\n");
    }

    uint8_t* ptr1 = MYMALLOC(SRAMCCM, 10);
    if (ptr1) {
        printf("ptr1 malloc ok, addr: %p\n", ptr1);
        printf("ptr1 memory use info: %d%%\n", mem_perused(SRAMCCM));
    } else {
        printf("ptr1 malloc fail\n");
    }

    for (uint8_t i = 0; i < 6; i++) {
        uint8_t* unused_ptr = MYMALLOC(SRAMEX1, 100);
        if (unused_ptr == NULL) {
            printf("%d malloc fail\n", i);
        }
    }

    uint8_t* refree_ptr = ptr;
    MYFREE(ptr);
    MYFREE(refree_ptr);

    atexit(exit_function);
    return 0;
}
