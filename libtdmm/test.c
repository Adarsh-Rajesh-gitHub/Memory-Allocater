#include "tdmm.h"


int main() {
    t_init(FIRST_FIT);
    printBlocks();
    void* ptr1 = t_malloc(100);
    printBlocks();
    void* ptr2 = t_malloc(200);
    printBlocks();
    t_free(ptr1);
    printBlocks();
    void* ptr3 = t_malloc(50);
    printBlocks();

    return 0;
}