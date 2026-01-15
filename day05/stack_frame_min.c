#include <stdio.h>

void foo(int x){
    int a = 10;
    int b = 20;

    printf("in foo:\n");
    printf("  &x = %p, x = %d\n", (void*)&x, x);
    printf("  &a = %p, a = %d\n", (void*)&a, a);
    printf("  &b = %p, b = %d\n", (void*)&b, b);
}

int main(void){
    int m = 777;

    printf("in main: \n");
    printf("  &m = %p, m = %d\n", (void*)&m, m);

    foo(123);
    foo(456);

    return 0;
}