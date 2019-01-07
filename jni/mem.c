#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "mem.h"
#include "opt.h"

bool zero_page_hash_ready = false;
uint32_t zero_page_hash = 0;

struct Process {
    int pid;
    int ppid;
    char *name;
    struct MemReg *maps;
    size_t maps_size;
    size_t maps_cap;
    FILE *pMapsFile;
    FILE *pMemFile;
    int pagemap_fd; // fd, we wiill use open(), not fopen().
    // some mem mem region has no name;
    // give it an unique name "[<no_name_cnt:start>]"
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

#define min_t(type, x, y) ({    \
    type __min1 = (x);          \
    type __min2 = (y);          \
    __min1 < __min2 ? __min1 : __min2; })

#define max_t(type, x, y) ({    \
    type __max1 = (x);          \
    type __max2 = (y);          \
    __max1 > __max2 ? __max1 : __max2;  })

static FILE* open_proc_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "fopen file: %s failed\n", filename);
        perror(NULL);
        exit(EXIT_FAILURE);
    }
    return f;
};

static int checked_open(const char *filename, int flags) {
    int fd = open(filename, flags);
    if (fd < 0) {
        perror(filename);
        exit(EXIT_FAILURE);
    }
    return fd;
}

static void open_proc_files(struct Process *p) {
    char file[128];
    sprintf(file, "/proc/%d/maps", p->pid);
    p->pMapsFile = open_proc_file(file);
    memset(file, 0, 128);
    sprintf(file, "/proc/%d/mem", p->pid);
    p->pMemFile = open_proc_file(file);
    memset(file, 0, 128);
    sprintf(file, "/proc/%d/pagemap", p->pid);
    p->pagemap_fd = checked_open(file, O_RDONLY);
}

// return NULL if failed
struct Process *proc_init(int pid) {
    struct Process *p = (struct Process *)malloc(sizeof(struct Process));
    p->pid = pid;
    p->no_name_cnt = 0;
    p->pMapsFile = NULL;
    p->pMemFile = NULL;
    p->pagemap_fd = -1;
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
    char r, w, x, s;
    unsigned long long pgoff;
    int major, minor; // device
    unsigned long inode;
    char path[256];
    memset(path, 0, 256);

    // format: addr-addr perms pgoff major:minor inode path
    int ret = sscanf(line, "%lx-%lx %c%c%c%c %llx %x:%x %lu %[^\n]\n",
                    &start_address, &end_address,
                    &r, &w, &x, &s,
                    &pgoff,
                    &major, &minor,
                    &inode,
                    path);
    if (ret < 10) {
        fprintf(stderr, "ret = %d\n", ret);
        fprintf(stderr, "%s: sscanf failed\n", __func__);
        // check by human, if correct
        fprintf(stderr, "%lx-%lx %c%c%c%c %llx %x:%x %lu %s\n",
                        start_address, end_address,
                        r, w, x, s,
                        pgoff,
                        major, minor,
                        inode,
                        path);
        fprintf(stderr, "%s\n", line);
        return;
    }

    m->start = start_address;
    m->end = end_address;
    // some vma doesn't have a nome, make one for it.
    if (strlen(path) != 0) {
        m->pathname = strdup(path);
    } else {
        sprintf(path, "[%d:%lx]", p->no_name_cnt++, m->start);
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
static void filter_anon_mr(struct Process *p) {
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
    // p->maps is a dynamicly allocated array, size double if full.
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

static unsigned long do_u64_read(int fd, const char *name, uint64_t *buf,
                                 unsigned long index, unsigned long count)
{
    long bytes;

    if (index > ULONG_MAX / 8) {
        fprintf(stderr, "index overflow: %lu\n", index);
        exit(EXIT_FAILURE);
    }
    bytes = pread(fd, buf, count * 8, (off_t)index * 8);
    if (bytes < 0) {
        perror(name);
        exit(EXIT_FAILURE);
    }
    if (bytes % 8) {
        fprintf(stderr, "partial read: %lu bytes\n", bytes);
        exit(EXIT_FAILURE);
    }
    return bytes / 8;
}

static unsigned long pagesmap_read(int fd, uint64_t *buf,
                                   unsigned long index, unsigned long pages)
{
    return do_u64_read(fd, "/proc/pid/pagemap", buf, index, pages);
}

#define PAGEMAP_BATCH	(64 << 10)
static void parse_pagemap(struct MemReg *m, int fd) {
    uint64_t buf[PAGEMAP_BATCH];
    // number of ?
    int count = m->end - m->start;
    unsigned long index = m->start;
    while (count) {
        unsigned long batch = min_t(unsigned long, count, PAGEMAP_BATCH);
        unsigned long pages = pagemap_read(fd, buf, index, batch);
        if (pages == 0)
            break;
       for (unsigned long i; i < pages; i++)  {
            //
       }
       index += pages;
       count -= pages;
    }
}


void proc_do(struct Process *p) {
    open_proc_files(p);
    parse_maps(p);
    // filter_small_mr(p);
    filter_anon_mr(p);

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
        fprintf(stderr, "Unable to attach to the process %d .\n", p->pid);
        perror(NULL);
        exit(EXIT_FAILURE);
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

static void init_zero_page_hash() {
    uint8_t page[pageSize];
    memset(page, 0, pageSize);
    zero_page_hash = page_hash(page, pageSize);
}

int page_is_zero(struct Page *p) {
    if (!zero_page_hash_ready)
        init_zero_page_hash();

    return p->hash == zero_page_hash;
}




