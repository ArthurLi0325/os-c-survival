#include <stdio.h>
#include <stdlib.h>

int* foo(){
    int *p = malloc(sizeof(int));
    if (!p){
        return NULL;
    }
    *p = 10;
    printf("in foo: p=%p, *p=%d\n", (void*)p, *p);
    return p; // OK: 回傳heap位址
}

int main(){
    int *p = foo();
    printf("int main: p=%p\n", (void*)p);

    if(p){
        printf("in main: *p=%d\n", *p);
        free(p); // 重要: 釋放
    }
    return 0;
}