/**
 * setcmp, compare a set of process. calculate redundancy.
 *
 *   for each <pid>, compute how many pages of it are not unique.
 *
 * usage:
 *    setcmp <pid> <pid> <pid> <pid> ... <pid>
 */
#include <stdio.h>
#include <stdlib.h>
#include "mem.h"
#include "simi.h"

// redunency
struct Redund
{
    int comm;
    struct MemReg *m1;
    struct MemReg *m2;
};

// compute similiarity between two memory regions

struct Set *build_set_from_mr(struct MemReg *m)
{
    struct Set *s = set_init();
    int n = mr_page_num(m);
    for (int i = 0; i < n; i++)
    {
        // filter zero page  ?
        struct Page *page = mr_get_page(m, i);
        if (page_is_zero(page))
            continue;
        set_add(s, data_init(page_to_u32(page)));
    }
    return s;
}

int comp_redunds(const void *a, const void *b)
{
    return ((struct Redund *)a)->comm - ((struct Redund *)b)->comm;
}

void print_simi(struct Process *p1, struct Process *p2)
{
    int n1 = proc_mr_num(p1);
    int n2 = proc_mr_num(p2);

    struct Set **sets1 = (struct Set **)malloc(sizeof(struct Set *) * n1);
    struct Set **sets2 = (struct Set **)malloc(sizeof(struct Set *) * n2);

    for (int i = 0; i < n1; i++)
    {
        struct MemReg *m = proc_get_mr(p1, i);
        sets1[i] = build_set_from_mr(m);
    }
    for (int i = 0; i < n2; i++)
    {
        struct MemReg *m = proc_get_mr(p2, i);
        sets2[i] = build_set_from_mr(m);
    }

    struct Redund *redunds = (struct Redund *)malloc(sizeof(struct Redund) * n1 * n2);
    if (redunds == NULL)
    {
        fprintf(stderr, "%s:%d, alloc failed\n", __func__, __LINE__);
        goto out1;
    }

    for (int i = 0; i < n1; i++)
    {
        for (int j = 0; j < n2; j++)
        {
            struct Redund *r = &redunds[i * n2 + j];
            r->comm = set_common(sets1[i], sets2[j]);
            r->m1 = proc_get_mr(p1, i);
            r->m2 = proc_get_mr(p2, j);
            // double simi = simi_mrs(m1, m2);
            // fprintf(stderr, "%lf \t %s, %s\n", simi, mr_get_name(m1), mr_get_name(m2));
        }
    }

    qsort(redunds, n1 * n2, sizeof(struct Redund), comp_redunds);

    for (int i = 0, total = n1 * n2; i < total; i++)
    {
        struct Redund *r = &redunds[i];
        if (r->comm == 0)
            continue;
        fprintf(stderr, "%d %s %lx %s %lx\n", r->comm,
                mr_get_name(r->m1), mr_get_start(r->m1),
                mr_get_name(r->m2), mr_get_start(r->m2));
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

#define MAX_PID 32
int main(int argc, char const *argv[])
{
    if (argc <= 2 || argc >= MAX_PID)
    {
        fprintf(stderr, "%s <pid> <pid> ...  <pid>\n", argv[0]);
        return -1;
    }

    int num_pid = argc - 1;
    int pid[MAX_PID];
    struct Process *proc[MAX_PID];
    for (int i = 0; i < num_pid; i++) {
        pid[i] = atoi(argv[i+1]);
        proc[i] = proc_init(pid[i]);
    }

    for (int i = 0; i < num_pid; i++) {
        fprintf(stderr, "%d\n", pid[i]);
        // proc_attach(proc[i]);
        // proc_do(proc[i]);
        // proc_detach(proc[i]);
    }

    // print_simi(p1, p2);

    for (int i = 0; i < num_pid; i++) {
        proc_del(proc[i]);
    }

    return 0;
}


