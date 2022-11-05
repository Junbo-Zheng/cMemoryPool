#include "./src/malloc.h"

#include <stdio.h>

int main(void)
{
    mymem_init(SRAMIN);
    mymem_init(SRAMEX);
    mymem_init(SRAMCCM);
    mymem_init(SRAMEX1);
    mymem_init(SRAMEX2);

    uint8_t *ptr = MYMALLOC(SRAMCCM, 12);
    if (ptr) {
        printf("ptr malloc ok, addr: %p\r\n", ptr);
        printf("ptr memory use info: %d%%\r\n", mem_perused(SRAMCCM));
    } else {
        printf("ptr malloc fail\n");
    }

    uint8_t *ptr1 = MYMALLOC(SRAMCCM, 10);
    if (ptr1) {
        printf("ptr1 malloc ok, addr: %p\n", ptr1);
        printf("ptr1 memory use info: %d%%\r\n", mem_perused(SRAMCCM));
    } else {
        printf("ptr1 malloc fail\n");
    }

    return 0;
}
