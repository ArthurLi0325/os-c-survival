#include <stdio.h>

int* bad_return(void){
    int x = 999;
    printf("in bad_return: &x=%p, x=%d\n", (void*)&x, x);
    return &x;
}

void clobber_stack(void){
    int a = 1, b = 2, c =3;
    printf("in clobber_stack: &a=%p, &b=%p, &c=%p\n", (void*)&a, (void*)&b, (void*)&c);
}

int main(void){
    int *p = bad_return();
    printf("after return: p=%p\n", (void*)p);

    clobber_stack();

    if(p){
        printf("after clobber: *p=%d (UB)\n", *p);
    }else{
        printf("p is NULL (still UB behavior from bad_return)\n");
    }

    return 0;
}