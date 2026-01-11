#include <stdio.h>
#include <stdlib.h>

int main(void){
    int *p = malloc(sizeof(int));
    if (!p){
        perror("malloc");
        return 1;
    }

    *p = 42;
    printf("[before free] p = %p, *p = %d\n", (void*)p, *p);

    free(p);
    printf("[after free]  p = %p\n", (void*)p);

    // use-after-free
    *p = 99;
    printf("[after write] *p = %d\n", (void*)p);

    return 0;
}