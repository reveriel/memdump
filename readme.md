# memdump

adapted from [memdump](https://security.stackexchange.com/questions/62300/memory-dumping-android/109068#109068
)

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





## question

- how to tell if a pages is in swap or not.