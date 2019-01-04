#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/ptrace.h>
#include "mem.h"
#include "opt.h"

struct Process {
    int pid;
    int ppid;
    char *name;
    struct MemReg *maps;
    size_t maps_size;
    size_t maps_cap;
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

struct Page {
    uint32_t hash;
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
    for (int i = 0; i < p->maps_size; i++) {
        free(p->maps[i].pathname);
    }
    free(p->maps);
}

// 'line' is one line from /proc/<pid>/maps
// parse 'line', save it to 'm'
static void parse_maps_line(char *line, struct MemReg* m) {
    unsigned long start_address;
    unsigned long end_address;
    char perms[5];
    int offset;
    char dev[8]; // some has value '2d' which is wired;
    int inode;
    char path[256];
    memset(path, 0, 256);
    memset(dev, 0, 8);

    // format: addr-addr perms offset major:minor inode path
    int ret = sscanf(line, "%lx-%lx %s %x %s %d %[^\n]\n",
                    &start_address, &end_address,
                    perms, &offset, dev, &inode, path);
    if (ret != 7 && ret != 6) {
        fprintf(stderr, "ret = %d\n", ret);
        fprintf(stderr, "%s: sscanf failed\n", __func__);
        // check by human, if correct
        fprintf(stderr, "%lx-%lx %s %x %s %d %s\n",
                        start_address, end_address,
                        perms, offset, dev, inode, path);
        fprintf(stderr, "%s\n", line);
        return;
    }

    m->start = start_address;
    m->end = end_address;
    if (strlen(path) != 0) {
        m->pathname = strdup(path);
    }
}

void proc_do(struct Process *p) {
    p->maps_size = 0;
    p->maps_cap = 4;
    p->maps = (struct MemReg*)malloc(p->maps_cap * sizeof(struct MemReg));

    char line[256];
    while (fgets(line, 256, p->pMapsFile) != NULL)
    {
        if (p->maps_cap == p->maps_size + 1) {
            p->maps_cap *= 2;
            p->maps = (struct MemReg*)realloc(p->maps, p->maps_cap * sizeof(struct MemReg));
        }
        parse_maps_line(line, &p->maps[p->maps_size]);
        p->maps_size++;
    }
}

// dump_memory_region(pMemFile, start_address, end_address - start_address, sock.fd);
void dump_memory_region_send(FILE* pMemFile, unsigned long start_address, long length, int serverSocket)
{
    unsigned long address;
    int pageLength = 4096;
    unsigned char page[pageLength];
    fseeko(pMemFile, start_address, SEEK_SET);

    for (address=start_address; address < start_address + length; address += pageLength)
    {
        fread(&page, 1, pageLength, pMemFile);
        if (serverSocket == -1)
        {
            // write to stdout
            fwrite(&page, 1, pageLength, stdout);
        }
        else
        {
            send(serverSocket, &page, pageLength, 0);
        }
    }
}


void proc_print_maps(struct Process *p) {
    for (int i = 0; i < p->maps_size; i++) {
        fprintf(stderr, "%08lx-%08lx \t %s\n", p->maps[i].start, p->maps[i].end,
                            p->maps[i].pathname);
    }
}

// attach to 'p', also wait until 'p' stops.
int proc_attach(struct Process *p) {
    long ptraceResult = ptrace(PTRACE_ATTACH, p->pid, NULL, NULL);
    if (ptraceResult < 0) {
        fprintf(stderr, "Unable to attach to the pid specified\n");
        return -1;
    }
    wait(NULL); // wait for tracee to stop
    return 0;
}

void proc_detach(struct Process *p) {
    ptrace(PTRACE_CONT, p->pid, NULL, NULL);
    ptrace(PTRACE_DETACH, p->pid, NULL, NULL);
}

