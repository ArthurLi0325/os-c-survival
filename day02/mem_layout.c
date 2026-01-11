#include <stdio.h>
#include <stdlib.h>

int global_var = 123;

int main(void){
    int local_var = 456;
    int *heap_var = malloc(sizeof(int));
    if (!heap_var){
        perror("malloc");
        return 1;
    }
    *heap_var = 789;

    printf("&global_var = %p\n", (void*)&global_var);
    printf("&local_var  = %p\n", (void*)&local_var);
    printf("heap_var    = %p\n", (void*)heap_var);
    printf("main        = %p\n", (void*)main);

    free(heap_var);
    return 0; 
}