#include <stdio.h>
#include <stdlib.h>

int main(void){
    int *p = malloc(sizeof(int));
    if (!p){
        perror("malloc");
        return 1;
    }

    *p = 42;
    printf("[p] addr = %p, *p = %d\n", (void*)p, *p);

    free(p);
    printf("[p] freed, p still = %p\n", (void*)p);

    int *q = malloc(sizeof(int));
    if (!q){
        perror("malloc");
        return 1;
    }
    *q = 777;

    printf("[q] addr = %p, *q = %d\n", (void*)q, *q);

    // dangling pointer aliasing
    printf("[UAF] p=%p, *p=%d (UNDEFINED)\n", (void*)p, *p);

    free(q);
    return 0;
}