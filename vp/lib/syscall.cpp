#include "syscall.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>

#include <iostream>
#include <stdexcept>

#include <boost/lexical_cast.hpp>



//see: riscv-gnu-toolchain/riscv-newlib/libgloss/riscv/
// for syscall implementation in the risc-v C lib (many are ignored and just return -1)

typedef int32_t reg_t;

typedef reg_t guest_long;


typedef int32_t rv32g_time_t;

struct rv32g_timespec {
    int32_t tv_sec;
    int32_t tv_nsec;
};

struct rv32g_stat
{
    uint64_t st_dev;
    uint64_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_rdev;
    uint64_t __pad1;
    int64_t st_size;
    int32_t st_blksize;
    int32_t __pad2;
    int64_t st_blocks;
    rv32g_timespec st_atim;
    rv32g_timespec st_mtim;
    rv32g_timespec st_ctim;
    int32_t __glibc_reserved[2];
};


void _copy_timespec(rv32g_timespec *dst, timespec *src) {
    dst->tv_sec  = src->tv_sec;
    dst->tv_nsec = src->tv_nsec;
}


int sys_fstat(SyscallHandler *sys, int fd, rv32g_stat *s_addr) {
    struct stat x;
    int ans = fstat(fd, &x);
    if (ans == 0) {
        rv32g_stat *p = (rv32g_stat *)sys->guest_to_host_pointer(s_addr);
        p->st_dev   = x.st_dev;
        p->st_ino   = x.st_ino;
        p->st_mode  = x.st_mode;
        p->st_nlink = x.st_nlink;
        p->st_uid   = x.st_uid;
        p->st_gid   = x.st_gid;
        p->st_rdev  = x.st_rdev;
        p->st_size  = x.st_size;
        p->st_blksize = x.st_blksize;
        p->st_blocks  = x.st_blocks;
        _copy_timespec(&p->st_atim, &x.st_atim);
        _copy_timespec(&p->st_mtim, &x.st_mtim);
        _copy_timespec(&p->st_ctim, &x.st_ctim);
    }
    return ans;
}


int sys_gettimeofday(SyscallHandler *sys, struct timeval *tp, void *tzp) {
    /*
     * timeval is using a struct with two long arguments.
     * The second argument tzp currently is not used by riscv code.
     */
    assert (tzp == 0);

    struct timeval x;
    int ans = gettimeofday(&x, 0);

    guest_long *p = (guest_long *)sys->guest_to_host_pointer(tp);
    p[0] = x.tv_sec;
    p[1] = x.tv_usec;
    return ans;
}


int sys_brk(SyscallHandler *sys, void *addr) {
    if (addr == 0) {
        return sys->hp;  // riscv newlib expects brk to return current heap address when zero is passed in
    } else {
        // NOTE: can also shrink again
        auto n = (uintptr_t)addr;
        sys->hp = n;

        if (sys->hp > sys->max_heap)
            sys->max_heap = sys->hp;

        return n;       // same for brk increase/decrease
    }
}


int sys_write(SyscallHandler *sys, int fd, const void *buf, size_t count) {
    const char *p = (const char *)sys->guest_to_host_pointer((void *)buf);

    auto ans = write(fd, p, count);

    assert (ans >= 0);

    return ans;
}


int sys_read(SyscallHandler *sys, int fd, void *buf, size_t count) {
    char *p = (char *)sys->guest_to_host_pointer(buf);

    auto ans = read(fd, p, count);

    assert (ans >= 0);

    return ans;
}


int sys_lseek(int fd, off_t offset, int whence) {
    auto ans = lseek(fd, offset, whence);

    return ans;
}


int sys_open(SyscallHandler *sys, const char *pathname, int flags, mode_t mode) {
    const char *host_pathname = (char *)sys->guest_to_host_pointer((void *)pathname);

    auto ans = open(host_pathname, flags, mode);

    std::cout << "[sys_open] " << host_pathname << ", " << flags << ", " << mode << std::endl;

    return ans;
}


int sys_close(int fd) {
    if (fd == STDOUT_FILENO || fd == STDIN_FILENO || fd == STDERR_FILENO) {
        // ignore closing of std streams, just return success
        return 0;
    } else {
        return close(fd);
    }
}


int sys_time(SyscallHandler *sys, rv32g_time_t *tloc) {

    time_t host_ans = time(0);

    rv32g_time_t guest_ans = boost::lexical_cast<rv32g_time_t>(host_ans);

    if (tloc != 0) {
        rv32g_time_t *p = (rv32g_time_t *)sys->guest_to_host_pointer(tloc);
        *p = guest_ans;
    }

    return guest_ans;
}


//TODO: add support for additional syscalls if necessary
int SyscallHandler::execute_syscall(ulong n, ulong _a0, ulong _a1, ulong _a2, ulong _a3) {
    //NOTE: when linking with CRT, the most basic example only calls *gettimeofday* and finally *exit*

    switch (n) {
        case SYS_fstat:
            return sys_fstat(this, _a0, (rv32g_stat *)_a1);

        case SYS_gettimeofday:
            return sys_gettimeofday(this, (struct timeval *)_a0, (void *)_a1);

        case SYS_brk:
            return sys_brk(this, (void *)_a0);

        case SYS_time:
            return sys_time(this, (rv32g_time_t *)_a0);

        case SYS_write:
            return sys_write(this, _a0, (void *)_a1, _a2);

        case SYS_read:
            return sys_read(this, _a0, (void *)_a1, _a2);

        case SYS_lseek:
            return sys_lseek(_a0, _a1, _a2);

        case SYS_open:
            return sys_open(this, (const char *)_a0, _a1, _a2);

        case SYS_close:
            return sys_close(_a0);

        case SYS_exit:
            shall_exit = true;
            return 0;

        case SYS_host_error:
            throw std::runtime_error("SYS_host_error");

        case SYS_host_test_pass:
            std::cout << "TEST_PASS" << std::endl;
            shall_exit = true;
            return 0;

        case SYS_host_test_fail:
            std::cout << "TEST_FAIL (testnum = " << _a0 << ")" << std::endl;
            shall_exit = true;
            return 0;
    }

    throw std::runtime_error("unsupported syscall '" + std::to_string(n) + "'");
}