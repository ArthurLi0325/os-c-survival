#include <stdio.h>

void inner(int depth){
    int x =42;
    printf("depth=%d &x=%p\n", depth, (void*)&x);
}

void outer(int depth){
    int y = 7;
    printf("depth=%d &y=%p\n", depth, (void*)&y);
    inner(depth);
}

int main(){
    for(int i = 0; i < 5; i++){
        outer(i);
    }
    return 0;
}