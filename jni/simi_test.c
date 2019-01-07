#include "simi.h"
#include <stdio.h>

static void _test(int *d_a, int a_size, int *d_b, int b_size)
{
    struct Set *a = set_init();
    struct Set *b = set_init();

    for (int i = 0; i < a_size; i++)
    {
        set_add(a, data_init(d_a[i]));
    }

    for (int i = 0; i < b_size; i++)
    {
        set_add(b, data_init(d_b[i]));
    }

    printf(" ===== test ======\n");
    printf("a:");
    set_print(a);
    printf("b:");
    set_print(b);

    printf(" a b simi  = %lf  \n", set_jaccard(a, b));
    printf(" a b comm  = %d  \n", set_common(a, b));

    set_free(a);
    set_free(b);
}

#define test(a, b) _test(a, ARRAY_SIZE(a), b, ARRAY_SIZE(b))

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

int main(int argc, char const *argv[])
{
    int a1[] = {1, 2, 3, 4, 5};
    int b1[] = {2, 3, 4, 5, 6};
    test(a1, b1);

    int a2[] = {1, 2, 2};
    int b2[] = {2, 2};
    test(a2, b2);

    return 0;
}
