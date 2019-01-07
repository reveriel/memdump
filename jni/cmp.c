/**
 * cmp : compare two process's memory region
 *
 * usage:
 *    cmp <pid> <pid>
 */
#include <stdio.h>
#include <stdlib.h>
#include "mem.h"
#include "simi.h"

// redunency
struct Redund {
    int comm;
    struct MemReg *m1;
    struct MemReg *m2;
};

// compute similiarity between two memory regions

struct Set *build_set_from_mr(struct MemReg *m) {
    struct Set *s =  set_init();
    int n = mr_page_num(m);
    for (int i = 0; i < n; i++) {
        // filter zero page  ?
        struct Page *page = mr_get_page(m, i);
        if (page_is_zero(page))
            continue;
        set_add(s, data_init(page_to_u32(page)));
    }
    return s;
}

int comp_redunds(const void *a, const void *b) {
    return ((struct Redund*)a)->comm - ((struct Redund *)b)->comm;
}

void print_simi(struct Process *p1, struct Process *p2) {
    int n1 = proc_mr_num(p1);
    int n2 = proc_mr_num(p2);

    struct Set **sets1 = (struct Set **)malloc(sizeof(struct Set *) * n1);
    struct Set **sets2 = (struct Set **)malloc(sizeof(struct Set *) * n2);

    for (int i = 0; i < n1; i++) {
        struct MemReg *m = proc_get_mr(p1, i);
        sets1[i] = build_set_from_mr(m);
    }
    for (int i = 0; i < n2; i++) {
        struct MemReg *m = proc_get_mr(p2, i);
        sets2[i] = build_set_from_mr(m);
    }

    struct Redund *redunds = (struct Redund *)malloc(sizeof(struct Redund) * n1 * n2);
    if (redunds == NULL) {
        fprintf(stderr, "%s:%d, alloc failed\n", __func__, __LINE__);
        goto out1;
    }

    for (int i = 0; i < n1; i++) {
        for (int j = 0; j < n2; j++) {
            struct Redund *r = &redunds[i * n2 + j];
            r->comm = set_common(sets1[i], sets2[j]);
            r->m1 = proc_get_mr(p1, i);
            r->m2 = proc_get_mr(p2, j);
            // double simi = simi_mrs(m1, m2);
            // fprintf(stderr, "%lf \t %s, %s\n", simi, mr_get_name(m1), mr_get_name(m2));
        }
    }

    qsort(redunds, n1 * n2, sizeof(struct Redund), comp_redunds);

    for (int i = 0, total = n1 * n2; i < total; i++) {
        struct Redund *r = &redunds[i];
        fprintf(stderr, "%d \t%s, %s\n", r->comm, mr_get_name(r->m1), mr_get_name(r->m2));
    }

    free(redunds);

out1:
    for (int i = 0; i < n1; i++)
        set_free(sets1[i]);
    for (int i = 0; i < n2; i++)
        set_free(sets2[i]);
    free(sets1);
    free(sets2);
}


int main(int argc, char const *argv[])
{
    int pid1, pid2;
    if (argc == 3) {
        pid1 = atoi(argv[1]);
        pid2 = atoi(argv[2]);
    } else {
        fprintf(stderr, "%s <pid> <pid>\n", argv[0]);
        return -1;
    }

    struct Process *p1 = proc_init(pid1);
    struct Process *p2 = proc_init(pid2);

    proc_attach(p1);
    proc_attach(p2);

    proc_do(p1);
    proc_do(p2);

    fprintf(stderr, "pid1 = %d, pid2 = %d\n", pid1, pid2);
    print_simi(p1, p2);

    proc_detach(p1);
    proc_detach(p2);

    proc_del(p1);
    proc_del(p2);
    return 0;
}




