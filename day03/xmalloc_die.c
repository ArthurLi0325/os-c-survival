#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

__attribute__((noreturn))
static void die(const char *fmt, ...){
    int saved_errno = errno; //先保存errno

    va_list ap;
    va_start(ap, fmt);

    fprintf(stderr, "fatal: ");
    vfprintf(stderr, fmt, ap);

    if (saved_errno != 0){
        fprintf(stderr, ": %s", strerror(saved_errno));
    }
    fprintf(stderr, "\n");

    va_end(ap);
    exit(1);
}

void* xmalloc(size_t n){
    void *p = malloc(n);
    if (!p){
        die("xmalloc(%zu) failed", n);
    }
    return p;
}

void* xcalloc(size_t nmemb, size_t size){
    void *p = calloc(nmemb, size);
    if (!p){
        die("xcalloc(%zu, %zu) failed", nmemb, size);
    }
    return p;
}

/*
 * realloc 的經典坑：
 *   ptr = realloc(ptr, newsize);
 * 若 realloc 失敗回 NULL，你會「丟掉原本 ptr」(memory leak + 失去指標)。
 *
 * 正確做法：用暫存指標接住成功結果，再回傳。
 */
 void* xrealloc(void *ptr, size_t newsize){
    void *tmp = realloc(ptr, newsize);
    if (!tmp){
        die("xrealloc(%p, %zu) failed", ptr, newsize);
    }
    return tmp;
 }

int main(void){
    // 1) xcalloc: 保證清成 0
    int *a = xcalloc(5, sizeof(int));
    for (int i = 0; i < 5; i++){
        printf("a[%d]=%d\n", i, a[i]);
    }

    a = xrealloc(a, 10 * sizeof(int));
    for (int i = 5; i < 10; i++){
        a[i] = i * 10;
    }

    for (int i = 0; i < 10; i++){
         printf("a[%d]=%d\n", i, a[i]);
    }

    free(a);
    return 0;
}