#include <stdint.h>
// #include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "mem.h"
#include "opt.h"

struct Process {
    int pid;
    int ppid;
    char *name;
    struct MemReg *maps;
    FILE *pMapsFile;
    FILE *pMemFile;
};

struct MemReg {
    unsigned long start;
    unsigned long end;
    char perms;
    char *pathname;
    struct Page *pages;
};


struct Process *proc_init(struct Option *opt) {
    struct Process *p = (struct Process *)malloc(sizeof(struct Process));
    p->pid = opt->pid;

    char mapsFilename[1024];
    sprintf(mapsFilename, "/proc/%d/maps", p->pid);
    p->pMapsFile = fopen(mapsFilename, "r");
    char memFilename[1024];
    sprintf(memFilename, "/proc/%d/mem", p->pid);
    p->pMemFile = fopen(memFilename, "r");

    return p;
}

void proc_del(struct Process *p) {
    fclose(p->pMapsFile);
    fclose(p->pMemFile);
}

void proc_do(struct Process *p) {
    char line[256];
    while (fgets(line, 256, p->pMapsFile) != NULL)
    {
        unsigned long start_address;
        unsigned long end_address;
        int ret = sscanf(line, "%08lx-%08lx\n", &start_address, &end_address);

        fprintf(stderr, "%s\n", line);
        // dump_memory_region(pMemFile, start_address, end_address - start_address, sock.fd);
    }
}
