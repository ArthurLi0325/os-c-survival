#include <stdio.h>

int main() {
    int x = 10;
    int *p = &x;

    printf("x  = %d\n", x);
    printf("&x = %p\n", (void*)&x);
    printf("p  = %p\n", (void*)p);
    printf("*p = %d\n", *p);

    *p = 99;
    printf("after *p=99, x = %d\n", x);
    return 0;
}
