#include <stdio.h>

int *foo(void){
    int x = 10;
    printf("[foo] &x = %p, x = %d\n", (void*)&x, x);
    return &x;   // 回傳 stack 上的位址（炸彈）
}

int main(void){
    int *p = foo();
    printf("[main] p  = %p\n", (void*)p);
    printf("[main] *p = %d\n", *p);
    return 0;
}