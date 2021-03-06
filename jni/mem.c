#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 500
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include "mem.h"
#include "opt.h"

bool zero_page_hash_ready = false;
uint32_t zero_page_hash = 0;
int kpageflags_fd = -1;
int kpagecount_fd = -1;
// not closed.. sorry

struct Timer {
    clock_t start;
    clock_t total;
} timer;

/*
 * pagemap kernel ABI bits
 */

#define PM_ENTRY_BYTES          8
#define PM_PFRAME_BITS	        55
#define PM_PFRAME_MASK          ((1LL << PM_PFRAME_BITS) - 1)
#define PM_PFRAME(x)            ((x) & PM_PFRAME_MASK)
#define MAX_SWAPFILES_SHIFT	    5
#define PM_SWAP_OFFSET(x)       (((x) & PM_PFRAME_MASK) >> MAX_SWAPFILES_SHIFT)
#define PM_SOFT_DIRTY	        (1ULL << 55)
#define PM_MMAP_EXCLUSIVE	    (1ULL << 56)
#define PM_FILE	                (1ULL << 61)
#define PM_SWAP                 (1ULL << 62)
#define PM_PRESENT              (1ULL << 63)

/*
 * Stable page flag bits exported to user space
 */

#define KPF_LOCKED              0
#define KPF_ERROR               1
#define KPF_REFERENCED          2
#define KPF_UPTODATE            3
#define KPF_DIRTY               4
#define KPF_LRU                 5
#define KPF_ACTIVE              6
#define KPF_SLAB                7
#define KPF_WRITEBACK           8
#define KPF_RECLAIM             9
#define KPF_BUDDY               10

/* 11-20: new additions in 2.6.31 */
#define KPF_MMAP                11
#define KPF_ANON                12
#define KPF_SWAPCACHE           13
#define KPF_SWAPBACKED          14
#define KPF_COMPOUND_HEAD       15
#define KPF_COMPOUND_TAIL       16
#define KPF_HUGE                17
#define KPF_UNEVICTABLE         18
#define KPF_HWPOISON            19
#define KPF_NOPAGE              20

#define KPF_KSM                 21
#define KPF_THP                 22
#define KPF_BALLOON             23
#define KPF_ZERO_PAGE           24
#define KPF_IDLE                25
#define KPF_PGTABLE             26

/* [32-] kernel hacking assistances */
#define KPF_RESERVED            32
#define KPF_MLOCKED             33
#define KPF_MAPPEDTODISK        34
#define KPF_PRIVATE             35
#define KPF_PRIVATE_2           36
#define KPF_OWNER_PRIVATE       37
#define KPF_ARCH                38
#define KPF_UNCACHED            39
#define KPF_SOFTDIRTY           40

/* [48-] take some arbitrary free slots for expanding overloaded flags
 * not part of kernel API
 */
#define KPF_READAHEAD           48
#define KPF_SLOB_FREE           49
#define KPF_SLUB_FROZEN         50
#define KPF_SLUB_DEBUG          51
#define KPF_FILE                61
#define KPF_SWAP                62
#define KPF_MMAP_EXCLUSIVE      63

#define KPF_ALL_BITS            ((uint64_t)~0ULL)
#define KPF_HACKERS_BITS        (0xffffULL << 32)
#define KPF_OVERLOADED_BITS     (0xffffULL << 48)
#define BIT(name)               (1ULL << KPF_##name)
#define BITS_COMPOUND           (BIT(COMPOUND_HEAD) | BIT(COMPOUND_TAIL))

static const char * const page_flag_names[] = {
    [KPF_LOCKED]            = "L:locked",
    [KPF_ERROR]             = "E:error",
    [KPF_REFERENCED]        = "R:referenced",
    [KPF_UPTODATE]          = "U:uptodate",
    [KPF_DIRTY]             = "D:dirty",
    [KPF_LRU]               = "l:lru",
    [KPF_ACTIVE]            = "A:active",
    [KPF_SLAB]              = "S:slab",
    [KPF_WRITEBACK]         = "W:writeback",
    [KPF_RECLAIM]           = "I:reclaim",
    [KPF_BUDDY]             = "B:buddy",

    [KPF_MMAP]              = "M:mmap",
    [KPF_ANON]              = "a:anonymous",
    [KPF_SWAPCACHE]         = "s:swapcache",
    [KPF_SWAPBACKED]        = "b:swapbacked",
    [KPF_COMPOUND_HEAD]     = "H:compound_head",
    [KPF_COMPOUND_TAIL]     = "T:compound_tail",
    [KPF_HUGE]              = "G:huge",
    [KPF_UNEVICTABLE]       = "u:unevictable",
    [KPF_HWPOISON]          = "X:hwpoison",
    [KPF_NOPAGE]            = "n:nopage",
    [KPF_KSM]               = "x:ksm",
    [KPF_THP]               = "t:thp",
    [KPF_BALLOON]           = "o:balloon",
    [KPF_PGTABLE]           = "g:pgtable",
    [KPF_ZERO_PAGE]         = "z:zero_page",
    [KPF_IDLE]              = "i:idle_page",

    [KPF_RESERVED]          = "r:reserved",
    [KPF_MLOCKED]           = "m:mlocked",
    [KPF_MAPPEDTODISK]      = "d:mappedtodisk",
    [KPF_PRIVATE]           = "P:private",
    [KPF_PRIVATE_2]         = "p:private_2",
    [KPF_OWNER_PRIVATE]     = "O:owner_private",
    [KPF_ARCH]              = "h:arch",
    [KPF_UNCACHED]          = "c:uncached",
    [KPF_SOFTDIRTY]         = "f:softdirty",

    [KPF_READAHEAD]         = "I:readahead",
    [KPF_SLOB_FREE]         = "P:slob_free",
    [KPF_SLUB_FROZEN]       = "A:slub_frozen",
    [KPF_SLUB_DEBUG]        = "E:slub_debug",

    [KPF_FILE]              = "F:file",
    [KPF_SWAP]              = "w:swap",
    [KPF_MMAP_EXCLUSIVE]    = "1:mmap_exclusive",
};

/**
 * a process
 */
struct Process
{
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

/**
 * a memory region, a vma in kernel
 */
struct MemReg
{
    unsigned long start;
    unsigned long end;
    char perms;
    char *pathname;
    struct Page *pages;
    char r, w, x, s;
};

struct Page
{
    uint32_t hash;
    int share; // how many pte are refering to this page;
};

const size_t pageSize = 4096;

#define ARRAY_SIZE(x) (sizeof(x) / (sizeof((x)[0])))

#define min_t(type, x, y) ({    \
    type __min1 = (x);          \
    type __min2 = (y);          \
    __min1 < __min2 ? __min1 : __min2; })

#define max_t(type, x, y) ({    \
    type __max1 = (x);          \
    type __max2 = (y);          \
    __max1 > __max2 ? __max1 : __max2; })

static void fatal(const char *x, ...)
{
	va_list ap;

	va_start(ap, x);
	vfprintf(stderr, x, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}


// https://en.wikipedia.org/wiki/Jenkins_hash_function
static uint32_t page_hash(uint8_t *page, size_t length)
{
    size_t i = 0;
    uint32_t hash = 0;
    while (i != length)
    {
        hash += page[i++];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

static FILE *open_proc_file(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (f == NULL)
    {
        fprintf(stderr, "fopen file: %s failed\n", filename);
        perror(NULL);
        exit(EXIT_FAILURE);
    }
    return f;
};

static int checked_open(const char *filename, int flags)
{
    int fd = open(filename, flags);
    if (fd < 0)
    {
        perror(filename);
        exit(EXIT_FAILURE);
    }
    return fd;
}

static void open_file_maps(struct Process *p)
{
    char file[128];
    sprintf(file, "/proc/%d/maps", p->pid);
    p->pMapsFile = open_proc_file(file);
}

static void open_proc_files(struct Process *p)
{
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

static void init_zero_page_hash()
{
    uint8_t page[pageSize];
    memset(page, 0, pageSize);
    zero_page_hash = page_hash(page, pageSize);
    zero_page_hash_ready = true;
}


// return NULL if failed
struct Process *proc_init(int pid)
{
    if (!zero_page_hash_ready)
        init_zero_page_hash();

    struct Process *p = (struct Process *)malloc(sizeof(struct Process));
    p->pid = pid;
    p->no_name_cnt = 0;
    p->pMapsFile = NULL;
    p->pMemFile = NULL;
    p->pagemap_fd = -1;
    return p;
}

void proc_del(struct Process *p)
{
    if (!p)
        return;
    if (p->pMapsFile)
        fclose(p->pMapsFile);
    if (p->pMemFile)
        fclose(p->pMemFile);
    if (p->pagemap_fd != -1)
        close(p->pagemap_fd);
    for (int i = 0; i < proc_mr_num(p); i++)
    {
        if (p->maps[i].pages)
            free(p->maps[i].pages);
        if (p->maps[i].pathname)
            free(p->maps[i].pathname);
    }
    free(p->maps);
}

// 'line' is one line from /proc/<pid>/maps
// parse 'line', save it to 'mr'
static void parse_maps_line(char *line, struct MemReg *mr)
{
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
    if (ret < 10)
    {
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

    mr->start = start_address;
    mr->end = end_address;
    mr->r = r;
    mr->w = w;
    mr->x = x;
    mr->s = s;
    // some vma doesn't have a nome, make one for it.
    if (strlen(path) != 0)
    {
        mr->pathname = strdup(path);
    }
    else
    {
        // sprintf(path, "[%d:%lx]", p->no_name_cnt++, mr->start);
        sprintf(path, "[0x%lx]", mr->start);
        mr->pathname = strdup(path);
    }
}

// return mr's permision string "rwxs"
char *mr_get_perm(struct MemReg *mr)
{
    static char str[5];
    str[0] = mr->r;
    str[1] = mr->w;
    str[2] = mr->x;
    str[3] = mr->s;
    return str;
}

// read all pages in.;
// save pages' hash
static void read_mem(struct Process *p)
{
    uint8_t page[pageSize];

    for (int i = 0; i < proc_mr_num(p); i++)
    {
        struct MemReg *mr = proc_get_mr(p, i);
        size_t numPages = (mr->end - mr->start) / pageSize + 1;
        mr->pages = (struct Page *)malloc(sizeof(struct Page) * numPages);

        fseeko(p->pMemFile, mr->start, SEEK_SET);
        int idx = 0;
        for (unsigned long addr = mr->start; addr < mr->end; addr += pageSize)
        {
            int ret = fread(page, 1, pageSize, p->pMemFile);
            if (ret != pageSize)
            {
                perror("fread error at /proc/pid/mem/");
                fprintf(stderr, "try again may solve the question\n");
                continue;
            }

            mr->pages[idx++].hash = page_hash(page, pageSize);

            if (idx == (int)numPages)
            {
                fprintf(stderr, "%s:pages size too small\n", __func__);
                break;
            }
        }
    }
}

inline bool mr_is_anon(struct MemReg *mr)
{
    return !mr->pathname || mr->pathname[0] == '[';
}

// if 'filter_anon, only save anonymous memory region
// memory region size(number of pages) <= 'minsize' will be ignored
static void parse_maps(struct Process *p, bool filter_anon, int minsize)
{

    // p->maps is a dynamicly allocated array, size double if full.
    p->maps_size = 0;
    p->maps_cap = 4;
    p->maps = (struct MemReg *)malloc(p->maps_cap * sizeof(struct MemReg));

    char line[256];
    while (fgets(line, 256, p->pMapsFile) != NULL)
    {
        if (p->maps_cap == p->maps_size + 1)
        {
        p->maps_cap *= 2;
            p->maps = (struct MemReg *)realloc(p->maps, p->maps_cap * sizeof(struct MemReg));
        }

        struct MemReg *mr = proc_get_mr(p, p->maps_size);

        parse_maps_line(line, mr);
        if (filter_anon && !mr_is_anon(mr))
            continue;
        if (mr_page_num(mr) <= minsize)
            continue;

        p->maps_size++;
    }
}

static char *page_flag_name(uint64_t flags)
{
    static char buf[65];
    size_t i, j;
    for (i = 0, j = 0; i < ARRAY_SIZE(page_flag_names); i++)
    {
        int present = (flags >> i) & 1;
        if (!page_flag_names[i])
        {
            if (present)
            {
                fprintf(stderr, "unknown flag bit %zu\n", i);
                exit(EXIT_FAILURE);
            }
            continue;
        }
        buf[j++] = present ? page_flag_names[i][0] : '-';
    }
    return buf;
}

// read from 'fd'
// put data in 'buf'
// start from 'index', 'count'
// return the number of u64 read in.
static unsigned long do_u64_read(int fd, const char *name, uint64_t *buf,
                                 unsigned long index, unsigned long count)
{
    if (index > ULONG_MAX / 8)
    {
        fprintf(stderr, "index overflow: %lu\n", index);
        exit(EXIT_FAILURE);
    }

    long bytes = pread(fd, buf, count * 8, (off_t)index * 8);

    if (bytes < 0)
    {
        perror(name);
        exit(EXIT_FAILURE);
    }
    if (bytes % 8)
    {
        fprintf(stderr, "partial read: %lu bytes\n", bytes);
        exit(EXIT_FAILURE);
    }
    return bytes / 8;
}

static unsigned long pagemap_read(int fd, uint64_t *buf,
                                  unsigned long index, unsigned long pages)
{
    return do_u64_read(fd, "/proc/pid/pagemap", buf, index, pages);
}

static unsigned long kpageflags_read(uint64_t *buf,
                                     unsigned long index, unsigned long pages)
{
    return do_u64_read(kpageflags_fd, "/proc/kpageflags", buf, index, pages);
}

static unsigned long kpagecount_read(uint64_t *buf,
                                     unsigned long index,
                                     unsigned long pages)
{
    return kpagecount_fd < 0 ? pages : do_u64_read(kpagecount_fd, "/proc/kpagecount", buf, index, pages);
}

static unsigned long pagemap_swap_offset(uint64_t val)
{
    return val & PM_SWAP ? PM_SWAP_OFFSET(val) : 0;
}

static unsigned long pagemap_pfn(uint64_t val)
{
    unsigned long pfn;

    if (val & PM_PRESENT)
        pfn = PM_PFRAME(val);
    else
        pfn = 0;

    return pfn;
}

static void show_page(unsigned long voffset, unsigned long offset,
                      uint64_t flags, uint64_t cnt)
{
    fprintf(stderr, "%ld %lx %lx \t%s\n", (int64_t)cnt, voffset, offset, page_flag_name(flags));
}

static uint64_t kpageflags_flags(uint64_t flags, uint64_t pme)
{
    /* SLOB/SLUB overload several page flags */
    if (flags & BIT(SLAB))
    {
        if (flags & BIT(PRIVATE))
            flags ^= BIT(PRIVATE) | BIT(SLOB_FREE);
        if (flags & BIT(ACTIVE))
            flags ^= BIT(ACTIVE) | BIT(SLUB_FROZEN);
        if (flags & BIT(ERROR))
            flags ^= BIT(ERROR) | BIT(SLUB_DEBUG);
    }

    /* PG_reclaim is overloaded as PG_readahead in the read path */
    if ((flags & (BIT(RECLAIM) | BIT(WRITEBACK))) == BIT(RECLAIM))
        flags ^= BIT(RECLAIM) | BIT(READAHEAD);

    if (pme & PM_SOFT_DIRTY)
        flags |= BIT(SOFTDIRTY);
    if (pme & PM_FILE)
        flags |= BIT(FILE);
    if (pme & PM_SWAP)
        flags |= BIT(SWAP);
    if (pme & PM_MMAP_EXCLUSIVE)
        flags |= BIT(MMAP_EXCLUSIVE);

    return flags;
}

static void print_page_flags_cnt(unsigned long voffset, unsigned long offset,
                     uint64_t flags, uint64_t cnt, uint64_t pme)
{
    flags = kpageflags_flags(flags, pme);
    show_page(voffset, offset, flags, cnt);
}


#define KPAGEFLAGS_BATCH (64 << 10)
static void walk_pfn(unsigned long voffset, unsigned long index,
                     unsigned long count, uint64_t pme,
                     struct MemReg *mr)
{
    uint64_t buf[KPAGEFLAGS_BATCH];
    uint64_t cnt[KPAGEFLAGS_BATCH];

    while (count)
    {
        unsigned long batch = min_t(unsigned long, count, KPAGEFLAGS_BATCH);
        unsigned long pages = kpageflags_read(buf, index, batch);
        if (pages == 0)
            break;

        if (kpagecount_read(cnt, index, pages) != pages)
            fatal("kpagecount returned fewer pages than expected");

        for (unsigned long i = 0; i < pages; i++)
        {
            // if (cnt[i] == 0)
            //     continue;

            // print_page_flags_cnt(voffset + i, index + i, buf[i], cnt[i], pme);

            // I need to feed the count data to 'mr'
            unsigned long vaddr = (voffset + i) * pageSize;
            // offset in mr->pages
            unsigned long page_off = (vaddr - mr->start) / pageSize;
            mr->pages[page_off].share = cnt[i];
        }

        index += pages;
        count -= pages;
    }
}

static void walk_swap(unsigned long voffset, uint64_t pme)
{
    uint64_t flags = kpageflags_flags(0, pme);
    show_page(voffset, pagemap_swap_offset(pme), flags, -1);
}

#define PAGEMAP_BATCH (64 << 8)
static void mr_parse_pagemap(struct MemReg *mr, int fd)
{
    // fprintf(stderr, "parse pagemap : %lx-%lx %s\n", mr->start, mr->end, mr->pathname);
    uint64_t buf[PAGEMAP_BATCH];

    unsigned long count = (mr->end - mr->start) / pageSize;
    unsigned long index = mr->start / pageSize;
    while (count)
    {
        // how many entry will we read.
        unsigned long batch = min_t(unsigned long, count, PAGEMAP_BATCH);
        // how many entry has been read
        unsigned long pages = pagemap_read(fd, buf, index, batch);
        if (pages == 0)
            break;
        for (unsigned long i = 0; i < pages; i++)
        {
            unsigned long pfn = pagemap_pfn(buf[i]);
            if (pfn)
                walk_pfn(index + i, pfn, 1, buf[i], mr); // I know it's not beautiful
            if (buf[i] & PM_SWAP)
                walk_swap(index + i, buf[i]);
        }
        index += pages;
        count -= pages;
    }
}

void proc_parse_pagemap(struct Process *p)
{
    kpageflags_fd = checked_open("/proc/kpageflags", O_RDONLY);
    kpagecount_fd = checked_open("/proc/kpagecount", O_RDONLY);

    for (int i = 0; i < proc_mr_num(p); i++)
    {
        proc_get_mr(p, i);
        struct MemReg *mr = proc_get_mr(p, i);
        mr_parse_pagemap(mr, p->pagemap_fd);
    }

    close(kpagecount_fd);
    close(kpagecount_fd);
}

void mem_time_report()
{
    fprintf(stderr, "read_mem time : %lf s\n", ((double)timer.total) / CLOCKS_PER_SEC );
}

static void time_start()
{
    timer.start = clock();
}

static void time_end()
{
    timer.total += clock() - timer.start;
    timer.start = 0;
}

void proc_do_maps(struct Process *p)
{
    open_file_maps(p);

    parse_maps(p,
               false, // only parse anonymous memory region
               0);   // page number <= 1, will be ignored
}

void proc_do(struct Process *p)
{
    open_proc_files(p);
    parse_maps(p,
               true, // only parse anonymous memory region
               0);   // page number <= 1, will be ignored

    // parse_pagemap(p);

    time_start();
    read_mem(p); // this is the most time consumming part
                    // more than 90%
    time_end();
}


void proc_print_maps(struct Process *p)
{
    fprintf(stderr, "start     -     end  nr_page \t name\n");
    for (int i = 0; i < proc_mr_num(p); i++)
    {
        struct MemReg *mr = proc_get_mr(p, i);
        fprintf(stderr, "%08lx-%08lx %d \t %s\n",
                mr->start, mr->end, mr_page_num(mr), mr->pathname);
    }
}

#define UNUSED(x) ((x) = (x))
// save proc to a file
void proc_save(const char *filename)
{
    UNUSED(filename);
}

struct Process *proc_load(const char *filename) {
    UNUSED(filename);
    return NULL;
}

static void mem_print_pages(struct MemReg *mr)
{
    int i = 0;
    for (unsigned long addr = mr->start; addr < mr->end; addr += pageSize)
    {
        if (mr->pages[i].share == 0)
        {
            i++;
            continue;
        }

        fprintf(stderr, "%d  %012lx: %08x\n",
                mr->pages[i].share, addr, mr->pages[i].hash);
        i++;
    }
}

void proc_print_pages(struct Process *p)
{
    for (int i = 0; i < proc_mr_num(p); i++)
    {
        struct MemReg *mr = proc_get_mr(p, i);
        fprintf(stderr, "%012lx-%012lx \t %s\n", mr->start, mr->end, mr->pathname);
        mem_print_pages(mr);
    }
}

// attach to 'p', also wait until 'p' stops.
// return 0 on success
int proc_attach(struct Process *p)
{
    long ptraceResult = ptrace(PTRACE_ATTACH, p->pid, NULL, NULL);
    if (ptraceResult < 0)
    {
        fprintf(stderr, "Unable to attach to the process %d .\n", p->pid);
        perror(NULL);
        exit(EXIT_FAILURE);
    }
    wait(NULL); // wait for tracee to stop
    return 0;
}

void proc_detach(struct Process *p)
{
    ptrace(PTRACE_CONT, p->pid, NULL, NULL);
    ptrace(PTRACE_DETACH, p->pid, NULL, NULL);
}

// return number of memory regions
inline int proc_mr_num(struct Process *p)
{
    return p->maps_size;
}

inline struct MemReg *proc_get_mr(struct Process *p, int index)
{
    return &p->maps[index];
}

inline int proc_get_pid(struct Process *p)
{
    return p->pid;
}

// return number of pages in mr
int mr_page_num(struct MemReg *mr)
{
    return (mr->end - mr->start) / pageSize;
}

struct Page *mr_get_page(struct MemReg *mr, int index)
{
    return &mr->pages[index];
}

const char *mr_get_name(struct MemReg *mr)
{
    return mr->pathname;
}

unsigned long mr_get_start(struct MemReg *mr)
{
    return mr->start;
}

uint32_t page_to_u32(struct Page *p)
{
    return p->hash;
}

int page_is_zero(struct Page *p)
{
    return p->hash == zero_page_hash;
}

int page_count(struct Page *p)
{
    return p->share;
}



