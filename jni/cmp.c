/**
 * cmp : compare two process's memory region
 *
 * usage:
 *    cmp <pid> <pid>
 */
#include <stdio.h>
// #include <stdlib.h>
#include "mem.h"
#include "simi.h"


// compute similiarity between two memory regions
double simi_mrs(struct MemReg *m1, struct MemReg *m2) {
    struct Set *s1 = set_init();
    struct Set *s2 = set_init();
    int n1 = mr_page_num(m1);
    int n2 = mr_page_num(m2);
    for (int i = 0; i < n1; i++) {
        set_add(s1, data_init(page_to_u32(mr_get_page(m1, i))));
    }
    for (int i = 0; i < n2; i++) {
        set_add(s2, data_init(page_to_u32(mr_get_page(m2, i))));
    }
    double simi = set_jaccard(s1, s2);

    set_free(s1);
    set_free(s2);
    return simi;
}

void print_simi(struct Process *p1, struct Process *p2) {
    int n1 = proc_mr_num(p1);
    int n2 = proc_mr_num(p2);

    for (int i = 0; i < n1; i++) {
            struct MemReg *m1 = proc_get_mr(p1, i);
        for (int j = 0; j < n2; j++) {
            struct MemReg *m2 = proc_get_mr(p2, j);
            double simi = simi_mrs(m1, m2);
            fprintf(stderr, "%lf \t %s, %s\n", simi, mr_get_name(m1), mr_get_name(m2));
        }
    }
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
    if (!p1 || !p2) {
        fprintf(stderr, "proc_init failed. exit\n");
        goto err;
    }

    if (proc_attach(p1)) {
        fprintf(stderr, "proc_attach p1 %d, failed. exit\n", pid1);
        goto err;
    }

    if (proc_attach(p2)) {
        fprintf(stderr, "proc_attach p2 %d, failed. exit\n", pid2);
        goto err;
    }

    proc_do(p1);
    proc_do(p2);

    fprintf(stderr, "pid1 = %d, pid2 = %d\n", pid1, pid2);
    print_simi(p1, p2);

    proc_detach(p1);
    proc_detach(p2);

    proc_del(p1);
    proc_del(p2);
    return 0;
err:
    proc_del(p1);
    proc_del(p2);
    return -1;
}






