#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/*
 * 回傳 0 表示成功，-1 表示失敗並設定 errno
 * out 由呼叫者提供，用來拿平均值
 */
static int avg_int_safe(const int *a, size_t n, int *out){
    if (out == NULL){
        errno = EINVAL;
        return -1;
    }
    if (a == NULL || n == 0){
        errno = EINVAL;
        return -1;
    }

    long sum = 0;
    for (size_t i = 0; i < n; i++){
        sum += a[i];
    }
    *out = (int)(sum / (long)n);
    return 0;
}

int main(void){
    int x[4] = {10, 20, 30, 40};
    int v;

    if (avg_int_safe(x, 4, &v) == 0){
        printf("avg(x,4) = %d\n", v);
    }else{
        perror("avg_int_safe(x,4) failed");
    }

    if (avg_int_safe(x, 0, &v) == 0){
        printf("avg(x,0) = %d\n", v);
    }else{
        perror("avg_int_safe(x,0) failed");
    }

    return 0;
}