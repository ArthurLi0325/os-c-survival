#include <stdio.h>

int* foo(){
    int x = 10;
    printf("in foo: &x=%p\n", (void*)&x);
    return &x; // 危險: 回傳stack上的位址
}

int main(){
    int *p = foo();
    printf("in main: p=%p", (void*)p);
    // printf("in main: *p=%d\n", *p);   // 暫時註解掉
    return 0;
}