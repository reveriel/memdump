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
    // some mem mem region has no name;
    // give it an unique name "[<no_name_cnt>]"
    int no_name_cnt;
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

const size_t pageSize = 4096;

// return 0 on success
static int open_files(struct Process *p) {
    char mapsFilename[1024];
    sprintf(mapsFilename, "/proc/%d/maps", p->pid);
    p->pMapsFile = fopen(mapsFilename, "r");
    if (p->pMapsFile == NULL) {
        fprintf(stderr, "fopen file: %s failed\n", mapsFilename);
        return 1;
    }
    char memFilename[1024];
    sprintf(memFilename, "/proc/%d/mem", p->pid);
    p->pMemFile = fopen(memFilename, "r");
    if (p->pMapsFile == NULL) {
        fprintf(stderr, "fopen file: %s failed\n", memFilename);
        fclose(p->pMapsFile);
        return 1;
    }
    return 0;
}

// return NULL if failed
struct Process *proc_init(int pid) {
    struct Process *p = (struct Process *)malloc(sizeof(struct Process));
    p->pid = pid;
    p->no_name_cnt = 0;
    if (open_files(p)) {
        free(p);
        return NULL;
    }
    return p;
}


void proc_del(struct Process *p) {
    if (!p)
        return;
    fclose(p->pMapsFile);
    fclose(p->pMemFile);
    for (int i = 0; i < p->maps_size - 1 ; i++) {
        if (p->maps[i].pages)
            free(p->maps[i].pages);
        if (p->maps[i].pathname)
            free(p->maps[i].pathname);
    }
    free(p->maps);
}

// 'line' is one line from /proc/<pid>/maps
// parse 'line', save it to 'm'
static void parse_maps_line(char *line, struct MemReg* m, struct Process *p) {
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
    } else {
        sprintf(path, "[%d]", p->no_name_cnt++);
        m->pathname = strdup(path);
    }
}

// https://en.wikipedia.org/wiki/Jenkins_hash_function
static uint32_t page_hash(uint8_t *page, size_t length) {
    size_t i = 0;
    uint32_t hash = 0;
    while (i != length) {
        hash += page[i++];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

// read all pages in.. anonymous vma;
static void read_mem(struct Process *p) {
    uint8_t page[pageSize];

    for (int i = 0; i < p->maps_size; i++) {
        struct MemReg *m = &p->maps[i];
        size_t numPages = (m->end - m->start) / pageSize + 1;
        m->pages = (struct Page *)malloc(sizeof(struct Page) * numPages);

        fseeko(p->pMemFile, m->start, SEEK_SET);
        int j = 0;
        for (unsigned long addr = m->start; addr < m->end; addr += pageSize) {
            int ret = fread(page, 1, pageSize, p->pMemFile);
            if (ret != pageSize) {
                fprintf(stderr, "fread error\n");
                continue;
            }
            m->pages[j++].hash = page_hash(page, pageSize);
            if (j == numPages)  {
                fprintf(stderr, "pages size too small\n");
                break;
            }
        }
    }
}


static bool is_anon(struct MemReg *m) {
    return !m->pathname || m->pathname[0] == '[';
}

// filter p->maps, filter out non-anonymous region
static void filter_anon(struct Process *p) {
    size_t anon_num = 0;
    // decide new p->maps's size;
    // fprintf(stderr, "maps size = %zu\n", p->maps_size);
    for (int i = 0; i < p->maps_size; i++) {
        struct MemReg *m = &p->maps[i];
        if (is_anon(m)) {
            anon_num++;
        }
    }
    struct MemReg *anonM = (struct MemReg *)malloc(sizeof(struct MemReg) * anon_num);
    int j = 0;
    for (int i = 0; i < p->maps_size; i++) {
        if (is_anon(&p->maps[i]))
            anonM[j++] = p->maps[i];
    }
    free(p->maps);
    p->maps_size = p->maps_cap = anon_num;
    p->maps = anonM;
}

static void parse_maps(struct Process *p) {
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
        parse_maps_line(line, &p->maps[p->maps_size], p);
        p->maps_size++;
    }
}

void proc_do(struct Process *p) {
    parse_maps(p);
    filter_anon(p);
    read_mem(p);
}

void proc_print_maps(struct Process *p) {
    for (int i = 0; i < p->maps_size; i++) {
        struct MemReg *m = &p->maps[i];
        fprintf(stderr, "%08lx-%08lx \t %s\n", m->start, m->end, m->pathname);
    }
}

static void mem_print_pages(struct MemReg *m) {
    int i = 0;
    for (unsigned long addr = m->start; addr < m->end; addr += pageSize) {
        fprintf(stderr, "  %012lx: %08x\n", addr, m->pages[i++].hash);
    }
}

void proc_print_pages(struct Process *p) {
    for (int i = 0; i < p->maps_size; i++) {
        struct MemReg *m = &p->maps[i];
        fprintf(stderr, "%012lx-%012lx \t %s\n", m->start, m->end, m->pathname);
        mem_print_pages(m);
    }
}

// attach to 'p', also wait until 'p' stops.
// return 0 on success
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


// return number of memory regions
int proc_mr_num(struct Process *p) {
    return p->maps_size;
}

struct MemReg *proc_get_mr(struct Process *p, int index) {
    return &p->maps[index];
}

// return number of pages in mr
int mr_page_num(struct MemReg *m) {
    return (m->end - m->start ) / pageSize;
}

struct Page *mr_get_page(struct MemReg *m, int index) {
    return &m->pages[index];
}

const char *mr_get_name(struct MemReg *m) {
    return m->pathname;
}

uint32_t page_to_u32(struct Page *p) {
    return p->hash;
}
