#include <stdio.h>

int* foo(){
    static int x = 10;
    printf("in foo: &x=%p, x=%d\n", (void*)&x, x);
    return &x; // OK: static 不會在foo return後消失
}

int main(){
    int *p = foo();
    printf("in main: p=%p, *p=%d\n", (void*)p, *p);

    *p = 99;        // 改掉static內容
    int *q = foo(); // 再呼叫一次
    printf("after modify: q=%p, *q=%d\n", (void*)q, *q);

    return 0;
}