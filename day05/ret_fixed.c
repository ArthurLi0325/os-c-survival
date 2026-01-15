#include <stdio.h>
#include <stdlib.h>

/* A) heap: caller owns it */
int* good_heap(void){
    int *p = malloc(sizeof(int));
    if(!p){
        return NULL;
    }
    *p = 999;
    printf("in good_heap: p=%p, *p=%d\n", (void*)p, *p);
    return p;
}

/* B) static: lifetime = whole program */
int* good_static(void){
    static int x = 999;
    printf("in good_static: &x=%p, x=%d\n", (void*)&x, x);
    return &x;
}

/* C) out-parameter: caller provides storage */
void good_outparam(int *out){
    *out = 999;
    printf("in good_outparam: out=%p, *out=%d\n", (void*)out, *out);
}

int main(void){
    /* A) heap */
    int *hp = good_heap();
    printf("after good_heap: hp=%p, *hp=%d\n", (void*)hp, hp ? *hp : -1);
    free(hp);

    puts("----");

    /* B) static */
    int *sp = good_static();
    printf("after good_static: sp=%p, *sp=%d\n", (void*)sp, *sp);

    puts("----");

    /* C) out-parameter */
    int m = 0;
    good_outparam(&m);
    printf("after good_outparam: &m=%p, m=%d\n", (void*)&m, m);

    return 0;
}