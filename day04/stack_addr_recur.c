#include <stdio.h>

void recur(int depth){
    int x = 123;
    char buf[64];   // 新增: 刻意吃stack
    buf[0] = (char)depth;   // 防止 buf 被最佳化掉
    printf("depth=%d &x=%p\n &buf=%p\n", depth, (void*)&x, (void*)buf);

    if(depth > 0){
        recur(depth - 1);
    }
}

int main(){
    recur(5);
    return 0;
}