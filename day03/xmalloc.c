#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

void* xmalloc(size_t n){
    void *p = malloc(n);
    if (p == NULL){
        // errno 通常會被 malloc 設成 ENOMEM（記憶體不足）
        fprintf(stderr, "xmalloc(%zu) failed: %s\n", n, strerror(errno));
        exit(1);
    }
    return p;
}

int main(void){
    int *a = xmalloc((size_t)1 << 62);

    for (int i = 0; i < 5; i++){
        a[i] = i * 10;
    }

    for (int i = 0; i < 5; i++){
        printf("a[%d] = %d\n", i, a[i]);
    }

    free(a);
    return 0;
}