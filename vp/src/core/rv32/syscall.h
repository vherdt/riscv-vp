#ifndef RISCV_ISA_SYSCALL_H
#define RISCV_ISA_SYSCALL_H

#include "stdint.h"
#include "assert.h"

//see: newlib/libgloss/riscv @ https://github.com/riscv/riscv-newlib/tree/riscv-newlib-2.5.0/libgloss/riscv

#define SYS_exit 93
#define SYS_exit_group 94
#define SYS_getpid 172
#define SYS_kill 129
#define SYS_read 63
#define SYS_write 64
#define SYS_open 1024
#define SYS_openat 56
#define SYS_close 57
#define SYS_lseek 62
#define SYS_brk 214
#define SYS_link 1025
#define SYS_unlink 1026
#define SYS_mkdir 1030
#define SYS_chdir 49
#define SYS_getcwd 17
#define SYS_stat 1038
#define SYS_fstat 80
#define SYS_lstat 1039
#define SYS_fstatat 79
#define SYS_access 1033
#define SYS_faccessat 48
#define SYS_pread 67
#define SYS_pwrite 68
#define SYS_uname 160
#define SYS_getuid 174
#define SYS_geteuid 175
#define SYS_getgid 176
#define SYS_getegid 177
#define SYS_mmap 222
#define SYS_munmap 215
#define SYS_mremap 216
#define SYS_time 1062
#define SYS_getmainvars 2011
#define SYS_rt_sigaction 134
#define SYS_writev 66
#define SYS_gettimeofday 169
#define SYS_times 153
#define SYS_fcntl 25
#define SYS_getdents 61
#define SYS_dup 23

// custom extensions
#define SYS_host_error 1        // indicate an error, i.e. this instruction should never be reached so something went wrong during exec.
#define SYS_host_test_pass 2    // RISC-V test execution successfully completed
#define SYS_host_test_fail 3    // RISC-V test execution failed



struct SyscallHandler {
    uint8_t *mem = 0;       // direct pointer to start of guest memory in host memory
    uint32_t mem_offset;    // start address of the memory as mapped into the address space
    uint32_t hp = 0;        // heap pointer
    bool shall_exit = false;

    // only for memory consumption evaluation
    uint32_t start_heap = 0;
    uint32_t max_heap = 0;

    uint32_t get_max_heap_memory_consumption() {
        return max_heap - start_heap;
    }


    void init(uint8_t *host_memory_pointer, uint32_t mem_start_address, uint32_t heap_pointer_address) {
        mem = host_memory_pointer;
        mem_offset = mem_start_address;
        hp = heap_pointer_address;

        start_heap = hp;
        max_heap = hp;
    }

    uint8_t *guest_address_to_host_pointer(uintptr_t addr) {
        assert (mem != nullptr);

        return mem + (addr - mem_offset);
    }

    uint8_t *guest_to_host_pointer(void *p) {
        return guest_address_to_host_pointer((uintptr_t)p);
    }

    typedef unsigned long int ulong;

    /*
     * Syscalls are implemented to work directly on guest memory (represented in host as byte array).
     * Note: the data structures on the host system might not be binary compatible with those on the guest system.
     */
    int execute_syscall(ulong n, ulong _a0, ulong _a1, ulong _a2, ulong _a3);
};


#endif //RISCV_ISA_SYSCALL_H
