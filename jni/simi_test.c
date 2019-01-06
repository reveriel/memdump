#include "simi.h"
#include <stdio.h>


int main(int argc, char const *argv[])
{
    struct Set *a = set_init();
    struct Set *b = set_init();
    int d_a[] = {1,2,3,4,5};
    int a_size = sizeof(d_a) / sizeof(int);
    int d_b[] = {2,3,4,5,6};
    int b_size = sizeof(d_b) / sizeof(int);
    for (int i = 0; i < a_size; i++) {
        set_add(a, data_init(d_a[i]));
    }

    for (int i = 0; i < b_size; i++) {
        set_add(b, data_init(d_b[i]));
    }

    printf("a:");
    set_print(a);
    printf("b:");
    set_print(b);

    printf(" a b simi  = %lf  \n", set_jaccard(a, b));

    set_free(a);
    set_free(b);
    return 0;
}


