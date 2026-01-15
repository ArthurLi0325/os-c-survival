#include <stdio.h>

__attribute__((noinline))
void foo(int x) {
    int a = 10;
    int b = 20;

    puts("== in foo ==");
    printf("&x=%p x=%d\n", (void*)&x, x);
    printf("&a=%p a=%d\n", (void*)&a, a);
    printf("&b=%p b=%d\n", (void*)&b, b);

    // 放一個停點位置（方便你在 gdb 裡 break 這行）
    puts("foo breakpoint spot");
}

int main(void) {
    int m = 777;

    puts("== in main ==");
    printf("&m=%p m=%d\n", (void*)&m, m);

    foo(123);
    foo(456);

    puts("done");
    return 0;
}