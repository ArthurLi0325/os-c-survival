#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static int avg_int(const int *a, size_t n){
    // invariant: a 不能是 NULL，n 必須 > 0
    assert(a != NULL);
    assert(n > 0);

    long sum = 0;
    for (size_t i = 0; i < n; i++){
        sum += a[i];
    }
    return (int)(sum / (long)n);
}

int main(void){
    int x[4] = {10, 20, 30, 40};

    printf("avg(x,4) = %d\n", avg_int(x, 4));

    // 刻意製造 bug：n=0（違反 invariant）
    printf("avg(x,0) = %d\n", avg_int(x, 0));

    return 0;
}