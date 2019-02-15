#pragma once

#include <stdint.h>


struct iss_syscall_if {
    virtual ~iss_syscall_if() {}

    virtual void sys_exit() = 0;
    //virtual void sys_break() = 0;

    virtual uint32_t read_register(unsigned idx) = 0;
    virtual void write_register(unsigned idx, uint32_t value) = 0;

    virtual uint32_t get_hart_id() = 0;
};


struct syscall_emulator_if {
    virtual ~syscall_emulator_if() {}

    virtual void execute_syscall(iss_syscall_if *core) = 0;
};