#include "./src/malloc.h"

int main(void)
{
    mymem_init(SRAMIN);
    mymem_init(SRAMEX);
    mymem_init(SRAMCCM);
    mymem_init(SRAMEX1);
    mymem_init(SRAMEX2);

    uint8_t *ptr = MYMALLOC(SRAMCCM, 12);
    printf("ptr is %p\r\n", ptr);

    printf("count is %d\r\n", mem_perused(SRAMCCM));

    uint8_t *ptr1 = MYMALLOC(SRAMCCM, 10);
    printf("ptr1 is %p\r\n", ptr1);

    printf("hello world\r\n");
    return 0x00;
}
