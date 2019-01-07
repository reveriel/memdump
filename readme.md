# memdump

adapted from [memdump](https://security.stackexchange.com/questions/62300/memory-dumping-android/109068#109068
)

? 用 Python 做数据分析不轻松多了吗???? 啊啊啊啊啊

## build

```
ndk-build
```

## test

1. build
2. send to device
3. run test on a simple native app


## /proc

一个应用有多个进程, 但 user (uid) 是相同的. 每启动一个应用都会有一个新的用户.
see https://developer.android.com/training/articles/security-tips
每个应用都被赋予唯一的 UID 和非常有限的权限.

see Documentation/filesystems/proc.txt

对我有用的:

```
fd             Directory, which contains all file descriptors
maps           Memory maps to executables and library files    (2.4)
mem            Memory held by this process
stat           Process status
statm          Process memory status information
status         Process status in human readable form
wchan          Present with CONFIG_KALLSYMS=y: it shows the kernel function
               symbol the task is blocked in - or "0" if not blocked.
pagemap        Page table
stack          Report full stack trace, enable via CONFIG_STACKTRACE
smaps          an extension based on maps, showing the memory consumption of
               each mapping and flags associated with it
numa_maps      an extension based on maps, showing the memory locality and
               binding policy as well as mem usage (in pages) of each mapping.
```

maps 每一行即一个内存区域.
see
[maps](https://stackoverflow.com/questions/1401359/understanding-linux-proc-id-maps)
and proc(5).
[proc.txt, android's
kernel](https://android.googlesource.com/kernel/msm/+/android-9.0.0_r0.41/Documentation/filesystems/proc.txt)


```
address           perms offset  dev   inode   pathname
08048000-08056000 r-xp 00000000 03:0c 64593   /usr/sbin/gpm
```

- address: address space, note: the address can be of  8/10/11/12 hex digits.
    which is 32/40/44/48 bits.
- perms: r = read, w = write, x = execute, s = shared, p = private(copy on write
- offset: offset into the file or whatever.
- device: device(major:minor)
  -    /dev/block/zram0 是多少? 这个能看出来吗?
  - mate20 中, 有些 minor 读出来是 2d, 不是个数字..
- inode - If the region was mapped from a file, this is the file number., the
  inode on that device. 0 indicates that no inode is associated with the memory
  region, like BSS.
- pathname: 如果是文件映射, 则是文件名, 否则可能是 psudo-paths:
  - [stack]: 进程 main stack
  - [stack:<tid>]:线程的 stack.
  - [vdso]:virtual dynamically liked shared object. see vdso(7).
  - [head]:堆
  - 其他, 在 android 上, 有很多一般linux 里没有的 pseudo-paths, 下面是测试了几个应用看到的:
    - [anon:bss]
    - [anon:System property context nodes]
    - [anon:arc4random data]
    - [anon:atexit handlers]
    - [anon:bionic TLS guard]
    - [anon:bionic TLS]
    - [anon:libc_malloc]
    - [anon:linker_alloc]
    - [anon:linker_alloc_lob]
    - [anon:linker_alloc_small_objects]
    - [anon:thread signal stack guard]
    - [anon:thread signal stack]
    - [anon:thread stack guard]
    - [vectors]
    - [anon:<name>]    = an anonymous mapping that has been named by userspace.
  - if empty, the mapping is anonymous.

## page table

The /proc/pid/pagemap gives the PFN, which can be used to find the pageflags
using /proc/kpageflags and number of times a page is mapped using
/proc/kpagecount. For detailed explanation, see
[Documentation/vm/pagemap.txt](https://android.googlesource.com/kernel/msm/+/android-9.0.0_r0.41/Documentation/vm/pagemap.txt).

不同 内核的 page flag 可能不一样, 最好看一个 fs/proc/page.c:kpageflags_read()
实现.

pagemap is a new (as of 2.6.25) set of interfaces in the kernel that allow
userspace programs to examine the page tables and related information by
reading files in /proc.
There are three components to pagemap:

 * /proc/pid/pagemap.  This file lets a userspace process find out which
   physical frame each virtual page is mapped to.  It contains one 64-bit
   value for each virtual page, containing the following data (from
   fs/proc/task_mmu.c, above pagemap_read):

    * Bits 0-54  page frame number (PFN) if present
    * Bits 0-4   swap type if swapped
    * Bits 5-54  swap offset if swapped
    * Bit  55    pte is soft-dirty (see Documentation/vm/soft-dirty.txt)
    * Bits 56-60 zero
    * Bit  61    page is file-page or shared-anon
    * Bit  62    page swapped
    * Bit  63    page present

   If the page is not present but in swap, then the PFN contains an
   encoding of the swap file number and the page's offset into the
   swap. Unmapped pages return a null PFN. This allows determining
   precisely which pages are mapped (or in swap) and comparing mapped
   pages between processes.
   Efficient users of this interface will use /proc/pid/maps to
   determine which areas of memory are actually mapped and llseek to
   skip over unmapped regions.
 * /proc/kpagecount.  This file contains a 64-bit count of the number of
   times each page is mapped, indexed by PFN.
 * /proc/kpageflags.  This file contains a 64-bit set of flags for each
   page, indexed by PFN.
   The flags are (from fs/proc/page.c, above kpageflags_read):
     0. LOCKED
     1. ERROR
     2. REFERENCED
     3. UPTODATE
     4. DIRTY
     5. LRU
     6. ACTIVE
     7. SLAB
     8. WRITEBACK
     9. RECLAIM
    10. BUDDY
    11. MMAP
    12. ANON
    13. SWAPCACHE
    14. SWAPBACKED
    15. COMPOUND_HEAD
    16. COMPOUND_TAIL
    16. HUGE
    18. UNEVICTABLE
    19. HWPOISON
    20. NOPAGE
    21. KSM
    22. THP

Short descriptions to the page flags:

 0. LOCKED
    page is being locked for exclusive access, eg. by undergoing read/write IO
 7. SLAB
    page is managed by the SLAB/SLOB/SLUB/SLQB kernel memory allocator
    When compound page is used, SLUB/SLQB will only set this flag on the head
    page; SLOB will not flag it at all.
10. BUDDY
    a free memory block managed by the buddy system allocator
    The buddy system organizes free memory in blocks of various orders.
    An order N block has 2^N physically contiguous pages, with the BUDDY flag
    set for and _only_ for the first page.
15. COMPOUND_HEAD
16. COMPOUND_TAIL
    A compound page with order N consists of 2^N physically contiguous pages.
    A compound page with order 2 takes the form of "HTTT", where H donates its
    head page and T donates its tail page(s).  The major consumers of compound
    pages are hugeTLB pages (Documentation/vm/hugetlbpage.txt), the SLUB etc.
    memory allocators and various device drivers. However in this interface,
    only huge/giga pages are made visible to end users.
17. HUGE
    this is an integral part of a HugeTLB page
19. HWPOISON
    hardware detected memory corruption on this page: don't touch the data!
20. NOPAGE
    no page frame exists at the requested address
21. KSM
    identical memory pages dynamically shared between one or more processes
22. THP
    contiguous pages which construct transparent hugepages

    [IO related page flags]
 1. ERROR     IO error occurred
 3. UPTODATE  page has up-to-date data
              ie. for file backed page: (in-memory data revision >= on-disk one)
 4. DIRTY     page has been written to, hence contains new data
              ie. for file backed page: (in-memory data revision >  on-disk one)
 8. WRITEBACK page is being synced to disk

    [LRU related page flags]
 5. LRU         page is in one of the LRU lists
 6. ACTIVE      page is in the active LRU list
18. UNEVICTABLE page is in the unevictable (non-)LRU list
                It is somehow pinned and not a candidate for LRU page reclaims,
		eg. ramfs pages, shmctl(SHM_LOCK) and mlock() memory segments
 2. REFERENCED  page has been referenced since last LRU list enqueue/requeue
 9. RECLAIM     page will be reclaimed soon after its pageout IO completed
11. MMAP        a memory mapped page
12. ANON        a memory mapped page that is not part of a file
13. SWAPCACHE   page is mapped to swap space, ie. has an associated swap entry
14. SWAPBACKED  page is backed by swap/RAM

The page-types tool in this directory can be used to query the above flags.

Using pagemap to do something useful:

The general procedure for using pagemap to find out about a process' memory
usage goes like this:

 1. Read /proc/pid/maps to determine which parts of the memory space are
    mapped to what.
 2. Select the maps you are interested in -- all of them, or a particular
    library, or the stack or the heap, etc.
 3. Open /proc/pid/pagemap and seek to the pages you would like to examine.
 4. Read a u64 for each page from pagemap.
 5. Open /proc/kpagecount and/or /proc/kpageflags.  For each PFN you just
    read, seek to that entry in the file, and read the data you want.

For example, to find the "unique set size" (USS), which is the amount of
memory that a process is using that is not shared with any other process,
you can go through every map in the process, find the PFNs, look those up
in kpagecount, and tally up the number of pages that are only referenced
once.

Other notes:

Reading from any of the files will return -EINVAL if you are not starting
the read on an 8-byte boundary (e.g., if you sought an odd number of bytes
into the file), or if the size of the read is not a multiple of 8 bytes.





## question

- how to tell if a pages is in swap or not.