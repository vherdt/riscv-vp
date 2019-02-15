#ifndef RISCV_ISA_REGISTERS_H
#define RISCV_ISA_REGISTERS_H

#include <iostream>
#include "string.h"
#include "assert.h"
#include "instr.h"

struct RegFile {
    enum {
        NUM_REGS = 32
    };

    int_rv_t regs[NUM_REGS]; //reg size is equal to xlen

    RegFile() {
        memset(regs, 0, sizeof(regs));
    }

    RegFile(const RegFile &other) {
        memcpy(regs, other.regs, sizeof(regs));
    }

    void write(uint32_t index, int_rv_t value) { //write xlen bit values
        assert (index <= x31);
        assert (index != x0);
        regs[index] = value;
    }

    int_rv_t read(uint32_t index) {//read xlen bits
        assert (index <= x31);
        return regs[index];
    }

    uint_rv_t shamt(uint32_t index) { //rv64 uses 5, 0 for shift amount (p.31)
        assert (index <= x31);
        return BIT_RANGE(regs[index], 5, 0);

    }

    int_rv_t &operator[](const uint32_t idx) {
        return regs[idx];
    }

    void show() {
        for (int i = 0; i < NUM_REGS; ++i) {
            std::cout << "r[" << i << "] = " << regs[i] << std::endl;
        }
    }

    enum e {
        x0 = 0,
        x1,
        x2,
        x3,
        x4,
        x5,
        x6,
        x7,
        x8,
        x9,
        x10,
        x11,
        x12,
        x13,
        x14,
        x15,
        x16,
        x17,
        x18,
        x19,
        x20,
        x21,
        x22,
        x23,
        x24,
        x25,
        x26,
        x27,
        x28,
        x29,
        x30,
        x31,

        zero = x0,
        ra = x1,
        sp = x2,
        gp = x3,
        tp = x4,
        t0 = x5,
        t1 = x6,
        t2 = x7,
        s0 = x8,
        fp = x8,
        s1 = x9,
        a0 = x10,
        a1 = x11,
        a2 = x12,
        a3 = x13,
        a4 = x14,
        a5 = x15,
        a6 = x16,
        a7 = x17,
        s2 = x18,
        s3 = x19,
        s4 = x20,
        s5 = x21,
        s6 = x22,
        s7 = x23,
        s8 = x24,
        s9 = x25,
        s10 = x26,
        s11 = x27,
        t3 = x28,
        t4 = x29,
        t5 = x30,
        t6 = x31,
    };
};
#endif //RISCV_ISA_REGISTERS_H
