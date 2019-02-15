#pragma once

#include <stdint.h>

struct syscall_if {
    virtual void sys_exit() = 0;

    virtual uint32_t read_register(unsigned idx) = 0;
    virtual void write_register(unsigned idx, uint32_t value) = 0;

    virtual uint32_t get_hart_id() = 0;
};