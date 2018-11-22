#ifndef RISCV_ISA_ISS_H
#define RISCV_ISA_ISS_H

#include "stdint.h"
#include "string.h"
#include "assert.h"

#include "memory.h"
#include "instr.h"
#include "bus.h"
#include "syscall.h"
#include "csr.h"
#include "irq_if.h"
#include "clint.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <map>
#include <vector>
#include <unordered_set>

#include <systemc>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/tlm_quantumkeeper.h>


struct RegFile {
    enum {
        NUM_REGS = 32
    };

    int32_t regs[NUM_REGS];

    RegFile() {
        memset(regs, 0, sizeof(regs));
    }

    RegFile(const RegFile &other) {
        memcpy(regs, other.regs, sizeof(regs));
    }

    void write(uint32_t index, int32_t value) {
        assert (index <= x31);
        assert (index != x0);
        regs[index] = value;
    }

    int32_t read(uint32_t index) {
        assert (index <= x31);
        return regs[index];
    }

    uint32_t shamt(uint32_t index) {
        assert (index <= x31);
        return BIT_RANGE(regs[index], 4, 0);
    }

    int32_t &operator [](const uint32_t idx) {
        return regs[idx];
    }

    void show() {
        for (int i=0; i<NUM_REGS; ++i) {
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



struct instr_memory_interface {
    virtual ~instr_memory_interface() {}

    virtual int32_t load_instr(uint32_t pc) = 0;
};


struct data_memory_interface {
    virtual ~data_memory_interface() {}

    virtual int32_t load_word(uint32_t addr) = 0;
    virtual int32_t load_half(uint32_t addr) = 0;
    virtual int32_t load_byte(uint32_t addr) = 0;
    virtual uint32_t load_uhalf(uint32_t addr) = 0;
    virtual uint32_t load_ubyte(uint32_t addr) = 0;

    virtual void store_word(uint32_t addr, uint32_t value) = 0;
    virtual void store_half(uint32_t addr, uint16_t value) = 0;
    virtual void store_byte(uint32_t addr, uint8_t value) = 0;
};


struct direct_memory_interface {
    uint8_t *mem;
    uint32_t offset;
    uint32_t size;
};


struct InstrMemoryProxy : public instr_memory_interface {
    direct_memory_interface &dmi;

    tlm_utils::tlm_quantumkeeper &quantum_keeper;
    sc_core::sc_time clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
    sc_core::sc_time access_delay = clock_cycle * 2;

    InstrMemoryProxy(direct_memory_interface &dmi, tlm_utils::tlm_quantumkeeper &keeper)
            : dmi(dmi), quantum_keeper(keeper) {
    }

    virtual int32_t load_instr(uint32_t pc) override {
        assert (pc >= dmi.offset);
        assert ((pc - dmi.offset) < dmi.size);

        quantum_keeper.inc(access_delay);

        return *((int32_t*)(dmi.mem + (pc - dmi.offset)));
    }
};


struct DataMemoryProxy : public data_memory_interface {
    /* Try to access the memory and redirect to the next data_memory_interface if the access is not in range */
    typedef uint32_t addr_t;

    direct_memory_interface &dmi;

    data_memory_interface *next_memory;

    tlm_utils::tlm_quantumkeeper &quantum_keeper;
    sc_core::sc_time clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
    sc_core::sc_time access_delay = clock_cycle * 4;

    DataMemoryProxy(direct_memory_interface &dmi, data_memory_interface *next_memory, tlm_utils::tlm_quantumkeeper &keeper)
        : dmi(dmi), next_memory(next_memory), quantum_keeper(keeper) {
    }

    template <typename T>
    inline T _load_data(addr_t addr) {
        if (addr >= dmi.offset && addr < dmi.size) {
            assert ((addr - dmi.offset + sizeof(T)) <= dmi.size);

            quantum_keeper.inc(access_delay);

            T ans = *((T*)(dmi.mem + (addr - dmi.offset)));
            return ans;
        } else {
            if (std::is_same<T, int8_t>::value) {
                return next_memory->load_byte(addr);
            } else if (std::is_same<T, int16_t>::value) {
                return next_memory->load_half(addr);
            } else if (std::is_same<T, int32_t>::value) {
                return next_memory->load_word(addr);
            } else if (std::is_same<T, uint16_t>::value) {
                return next_memory->load_uhalf(addr);
            } else if (std::is_same<T, uint8_t>::value) {
                return next_memory->load_ubyte(addr);
            } else {
                assert(false && "unsupported load operation");
            }
        }
    }

    template <typename T>
    inline void _store_data(addr_t addr, T value) {
        if (addr >= dmi.offset && addr < dmi.size) {
            assert ((addr - dmi.offset + sizeof(T)) <= dmi.size);

            quantum_keeper.inc(access_delay);

            *((T*)(dmi.mem + (addr - dmi.offset))) = value;
        } else {
            if (std::is_same<T, uint8_t>::value) {
                next_memory->store_byte(addr, value);
            } else if (std::is_same<T, uint16_t>::value) {
                next_memory->store_half(addr, value);
            } else if (std::is_same<T, uint32_t>::value) {
                next_memory->store_word(addr, value);
            } else {
                assert(false && "unsupported store operation");
            }
        }
    }

    virtual int32_t load_word(addr_t addr) { return _load_data<int32_t>(addr); }
    virtual int32_t load_half(addr_t addr) { return _load_data<int16_t>(addr); }
    virtual int32_t load_byte(addr_t addr) { return _load_data<int8_t>(addr); }
    virtual uint32_t load_uhalf(addr_t addr) { return _load_data<uint16_t>(addr); }
    virtual uint32_t load_ubyte(addr_t addr) { return _load_data<uint8_t>(addr); }

    virtual void store_word(addr_t addr, uint32_t value) { _store_data(addr, value); }
    virtual void store_half(addr_t addr, uint16_t value) { _store_data(addr, value); }
    virtual void store_byte(addr_t addr, uint8_t value) { _store_data(addr, value); }
};


struct CombinedMemoryInterface : public sc_core::sc_module,
                                 public instr_memory_interface,
                                 public data_memory_interface {
    typedef uint32_t addr_t;

    tlm_utils::simple_initiator_socket<CombinedMemoryInterface> isock;
    tlm_utils::tlm_quantumkeeper &quantum_keeper;

    CombinedMemoryInterface(sc_core::sc_module_name, tlm_utils::tlm_quantumkeeper &keeper)
        : quantum_keeper(keeper) {
    }

    inline void _do_transaction(tlm::tlm_command cmd, uint64_t addr, uint8_t *data, unsigned num_bytes) {
        tlm::tlm_generic_payload trans;
        trans.set_command(cmd);
        trans.set_address(addr);
        trans.set_data_ptr(data);
        trans.set_data_length(num_bytes);

        sc_core::sc_time local_delay = quantum_keeper.get_local_time();

        isock->b_transport(trans, local_delay);

        assert (local_delay >= quantum_keeper.get_local_time());
        quantum_keeper.set(local_delay);
    }

    template <typename T>
    inline T _load_data(addr_t addr) {
        T ans;
        _do_transaction(tlm::TLM_READ_COMMAND, addr, (uint8_t*)&ans, sizeof(T));
        return ans;
    }

    template <typename T>
    inline void _store_data(addr_t addr, T value) {
        _do_transaction(tlm::TLM_WRITE_COMMAND, addr, (uint8_t*)&value, sizeof(T));
    }

    int32_t load_instr(addr_t addr) { return _load_data<int32_t>(addr); }

    int32_t load_word(addr_t addr) { return _load_data<int32_t>(addr); }
    int32_t load_half(addr_t addr) { return _load_data<int16_t>(addr); }
    int32_t load_byte(addr_t addr) { return _load_data<int8_t>(addr); }
    uint32_t load_uhalf(addr_t addr) { return _load_data<uint16_t>(addr); }
    uint32_t load_ubyte(addr_t addr) { return _load_data<uint8_t>(addr); }

    void store_word(addr_t addr, uint32_t value) { _store_data(addr, value); }
    void store_half(addr_t addr, uint16_t value) { _store_data(addr, value); }
    void store_byte(addr_t addr, uint8_t value) { _store_data(addr, value); }
};


enum class CoreExecStatus {
    Runnable,
    HitBreakpoint,
    Terminated,
};


struct ISS : public sc_core::sc_module,
             public external_interrupt_target,
             public timer_interrupt_target {

    clint_if *clint = nullptr;
    instr_memory_interface *instr_mem = nullptr;
    data_memory_interface *mem = nullptr;
    SyscallHandler *sys = nullptr;
    RegFile regs;
    uint32_t pc;
    uint32_t last_pc;
    csr_table csrs;
    uint32_t lrw_marked = 0;

    CoreExecStatus status = CoreExecStatus::Runnable;
    std::unordered_set<uint32_t> breakpoints;
    bool debug_mode = false;

    sc_core::sc_event wfi_event;

    tlm_utils::tlm_quantumkeeper quantum_keeper;
    sc_core::sc_time cycle_time;
    std::array<sc_core::sc_time, Opcode::NUMBER_OF_INSTRUCTIONS> instr_cycles;

    enum {
        REG_MIN = INT32_MIN,
    };

    ISS()
        : sc_module(sc_core::sc_module_name("ISS")) {

        sc_core::sc_time qt = tlm::tlm_global_quantum::instance().get();
        cycle_time = sc_core::sc_time(10, sc_core::SC_NS);

        assert (qt >= cycle_time);
        assert (qt % cycle_time == sc_core::SC_ZERO_TIME);

        for (int i=0; i<Opcode::NUMBER_OF_INSTRUCTIONS; ++i)
            instr_cycles[i] = cycle_time;

        const sc_core::sc_time memory_access_cycles = 4*cycle_time;
        const sc_core::sc_time mul_div_cycles = 8*cycle_time;

        instr_cycles[Opcode::LB] = memory_access_cycles;
        instr_cycles[Opcode::LBU] = memory_access_cycles;
        instr_cycles[Opcode::LH] = memory_access_cycles;
        instr_cycles[Opcode::LHU] = memory_access_cycles;
        instr_cycles[Opcode::LW] = memory_access_cycles;
        instr_cycles[Opcode::SB] = memory_access_cycles;
        instr_cycles[Opcode::SH] = memory_access_cycles;
        instr_cycles[Opcode::SW] = memory_access_cycles;
        instr_cycles[Opcode::MUL] = mul_div_cycles;
        instr_cycles[Opcode::MULH] = mul_div_cycles;
        instr_cycles[Opcode::MULHU] = mul_div_cycles;
        instr_cycles[Opcode::MULHSU] = mul_div_cycles;
        instr_cycles[Opcode::DIV] = mul_div_cycles;
        instr_cycles[Opcode::DIVU] = mul_div_cycles;
        instr_cycles[Opcode::REM] = mul_div_cycles;
        instr_cycles[Opcode::REMU] = mul_div_cycles;
    }

    Opcode::mapping exec_step() {
        auto mem_word = instr_mem->load_instr(pc);
        Instruction instr(mem_word);
        Opcode::mapping op;
        if (instr.is_compressed()) {
            op = instr.decode_and_expand_compressed();
            pc += 2;
        } else {
            op = instr.decode_normal();
            pc += 4;
        }

        switch (op) {
            case Opcode::UNDEF:
                throw std::runtime_error("unknown instruction");

            case Opcode::ADDI:
                regs[instr.rd()] = regs[instr.rs1()] + instr.I_imm();
                break;

            case Opcode::SLTI:
                regs[instr.rd()] = regs[instr.rs1()] < instr.I_imm() ? 1 : 0;
                break;

            case Opcode::SLTIU:
                regs[instr.rd()] = ((uint32_t)regs[instr.rs1()]) < ((uint32_t)instr.I_imm()) ? 1 : 0;
                break;

            case Opcode::XORI:
                regs[instr.rd()] = regs[instr.rs1()] ^ instr.I_imm();
                break;

            case Opcode::ORI:
                regs[instr.rd()] = regs[instr.rs1()] | instr.I_imm();
                break;

            case Opcode::ANDI:
                regs[instr.rd()] = regs[instr.rs1()] & instr.I_imm();
                break;

            case Opcode::ADD:
                regs[instr.rd()] = regs[instr.rs1()] + regs[instr.rs2()];
                break;

            case Opcode::SUB:
                regs[instr.rd()] = regs[instr.rs1()] - regs[instr.rs2()];
                break;

            case Opcode::SLL:
                regs[instr.rd()] = regs[instr.rs1()] << regs.shamt(instr.rs2());
                break;

            case Opcode::SLT:
                regs[instr.rd()] = regs[instr.rs1()] < regs[instr.rs2()];
                break;

            case Opcode::SLTU:
                regs[instr.rd()] = ((uint32_t)regs[instr.rs1()]) < ((uint32_t)regs[instr.rs2()]);
                break;

            case Opcode::SRL:
                regs[instr.rd()] = ((uint32_t)regs[instr.rs1()]) >> regs.shamt(instr.rs2());
                break;

            case Opcode::SRA:
                regs[instr.rd()] = regs[instr.rs1()] >> regs.shamt(instr.rs2());
                break;

            case Opcode::XOR:
                regs[instr.rd()] = regs[instr.rs1()] ^ regs[instr.rs2()];
                break;

            case Opcode::OR:
                regs[instr.rd()] = regs[instr.rs1()] | regs[instr.rs2()];
                break;

            case Opcode::AND:
                regs[instr.rd()] = regs[instr.rs1()] & regs[instr.rs2()];
                break;

            case Opcode::SLLI:
                regs[instr.rd()] = regs[instr.rs1()] << instr.shamt();
                break;

            case Opcode::SRLI:
                regs[instr.rd()] = ((uint32_t)regs[instr.rs1()]) >> instr.shamt();
                break;

            case Opcode::SRAI:
                regs[instr.rd()] = regs[instr.rs1()] >> instr.shamt();
                break;

            case Opcode::LUI:
                regs[instr.rd()] = instr.U_imm();
                break;

            case Opcode::AUIPC:
                regs[instr.rd()] = last_pc + instr.U_imm();
                break;

            case Opcode::JAL:
                if (instr.rd() != RegFile::zero)
                    regs[instr.rd()] = pc;
                pc = last_pc + instr.J_imm();
                break;

            case Opcode::JALR: {
                uint32_t link = pc;
                pc = (regs[instr.rs1()] + instr.I_imm()) & ~1;
                if (instr.rd() != RegFile::zero)
                    regs[instr.rd()] = link;
            }
                break;

            case Opcode::SB: {
                uint32_t addr = regs[instr.rs1()] + instr.S_imm();
                mem->store_byte(addr, regs[instr.rs2()]);
            }
                break;

            case Opcode::SH: {
                uint32_t addr = regs[instr.rs1()] + instr.S_imm();
                mem->store_half(addr, regs[instr.rs2()]);
            }
                break;

            case Opcode::SW: {
                uint32_t addr = regs[instr.rs1()] + instr.S_imm();
                mem->store_word(addr, regs[instr.rs2()]);
            }
                break;

            case Opcode::LB: {
                uint32_t addr = regs[instr.rs1()] + instr.I_imm();
                regs[instr.rd()] = mem->load_byte(addr);
            }
                break;

            case Opcode::LH: {
                uint32_t addr = regs[instr.rs1()] + instr.I_imm();
                regs[instr.rd()] = mem->load_half(addr);
            }
                break;

            case Opcode::LW: {
                uint32_t addr = regs[instr.rs1()] + instr.I_imm();
                regs[instr.rd()] = mem->load_word(addr);
            }
                break;

            case Opcode::LBU: {
                uint32_t addr = regs[instr.rs1()] + instr.I_imm();
                regs[instr.rd()] = mem->load_ubyte(addr);
            }
                break;

            case Opcode::LHU: {
                uint32_t addr = regs[instr.rs1()] + instr.I_imm();
                regs[instr.rd()] = mem->load_uhalf(addr);
            }
                break;

            case Opcode::BEQ:
                if (regs[instr.rs1()] == regs[instr.rs2()])
                    pc = last_pc + instr.B_imm();
                break;

            case Opcode::BNE:
                if (regs[instr.rs1()] != regs[instr.rs2()])
                    pc = last_pc + instr.B_imm();
                break;

            case Opcode::BLT:
                if (regs[instr.rs1()] < regs[instr.rs2()])
                    pc = last_pc + instr.B_imm();
                break;

            case Opcode::BGE:
                if (regs[instr.rs1()] >= regs[instr.rs2()])
                    pc = last_pc + instr.B_imm();
                break;

            case Opcode::BLTU:
                if ((uint32_t)regs[instr.rs1()] < (uint32_t)regs[instr.rs2()])
                    pc = last_pc + instr.B_imm();
                break;

            case Opcode::BGEU:
                if ((uint32_t)regs[instr.rs1()] >= (uint32_t)regs[instr.rs2()])
                    pc = last_pc + instr.B_imm();
                break;

            case Opcode::FENCE: {
                // not using out of order execution so can be ignored
                break;
            }

            case Opcode::ECALL: {
                // NOTE: cast to unsigned value to avoid sign extension, since execute_syscall expects a native 64 bit value
                int ans = sys->execute_syscall((uint32_t)regs[RegFile::a7], (uint32_t)regs[RegFile::a0], (uint32_t)regs[RegFile::a1], (uint32_t)regs[RegFile::a2], (uint32_t)regs[RegFile::a3]);
                regs[RegFile::a0] = ans;
            } break;

            case Opcode::EBREAK:
                status = CoreExecStatus::HitBreakpoint;
                break;

            case Opcode::CSRRW: {
                auto rd = instr.rd();
                auto rs1_val = regs[instr.rs1()];
                auto &csr = csr_update_and_get(instr.csr());
                if (rd != RegFile::zero) {
                    regs[instr.rd()] = csr.read();
                }
                csr.write(rs1_val);
            } break;

            case Opcode::CSRRS: {
                auto rd = instr.rd();
                auto rs1 = instr.rs1();
                auto rs1_val = regs[instr.rs1()];
                auto &csr = csr_update_and_get(instr.csr());
                if (rd != RegFile::zero)
                    regs[rd] = csr.read();
                if (rs1 != RegFile::zero)
                    csr.set_bits(rs1_val);
            } break;

            case Opcode::CSRRC: {
                auto rd = instr.rd();
                auto rs1 = instr.rs1();
                auto rs1_val = regs[instr.rs1()];
                auto &csr = csr_update_and_get(instr.csr());
                if (rd != RegFile::zero)
                    regs[rd] = csr.read();
                if (rs1 != RegFile::zero)
                    csr.clear_bits(rs1_val);
            } break;

            case Opcode::CSRRWI: {
                auto rd = instr.rd();
                auto &csr = csr_update_and_get(instr.csr());
                if (rd != RegFile::zero) {
                    regs[rd] = csr.read();
                }
                csr.write(instr.zimm());
            } break;

            case Opcode::CSRRSI: {
                auto rd = instr.rd();
                auto zimm = instr.zimm();
                auto &csr = csr_update_and_get(instr.csr());
                if (rd != RegFile::zero)
                    regs[rd] = csr.read();
                if (zimm != 0)
                    csr.set_bits(zimm);
            } break;

            case Opcode::CSRRCI: {
                auto rd = instr.rd();
                auto zimm = instr.zimm();
                auto &csr = csr_update_and_get(instr.csr());
                if (rd != RegFile::zero)
                    regs[rd] = csr.read();
                if (zimm != 0)
                    csr.clear_bits(zimm);
            } break;


            case Opcode::MUL: {
                int64_t ans = (int64_t)regs[instr.rs1()] * (int64_t)regs[instr.rs2()];
                regs[instr.rd()] = ans & 0xFFFFFFFF;
            } break;

            case Opcode::MULH: {
                int64_t ans = (int64_t)regs[instr.rs1()] * (int64_t)regs[instr.rs2()];
                regs[instr.rd()] = (ans & 0xFFFFFFFF00000000) >> 32;
            } break;

            case Opcode::MULHU: {
                int64_t ans = ((uint64_t)(uint32_t)regs[instr.rs1()]) * (uint64_t)((uint32_t)regs[instr.rs2()]);
                regs[instr.rd()] = (ans & 0xFFFFFFFF00000000) >> 32;
            } break;

            case Opcode::MULHSU: {
                int64_t ans = (int64_t)regs[instr.rs1()] * (uint64_t)((uint32_t)regs[instr.rs2()]);
                regs[instr.rd()] = (ans & 0xFFFFFFFF00000000) >> 32;
            } break;

            case Opcode::DIV: {
                auto a = regs[instr.rs1()];
                auto b = regs[instr.rs2()];
                if (b == 0) {
                    regs[instr.rd()] = -1;
                } else if (a == REG_MIN && b == -1) {
                    regs[instr.rd()] = a;
                } else {
                    regs[instr.rd()] = a / b;
                }
            } break;

            case Opcode::DIVU: {
                auto a = regs[instr.rs1()];
                auto b = regs[instr.rs2()];
                if (b == 0) {
                    regs[instr.rd()] = -1;
                } else {
                    regs[instr.rd()] = (uint32_t)a / (uint32_t)b;
                }
            } break;

            case Opcode::REM: {
                auto a = regs[instr.rs1()];
                auto b = regs[instr.rs2()];
                if (b == 0) {
                    regs[instr.rd()] = a;
                } else if (a == REG_MIN && b == -1) {
                    regs[instr.rd()] = 0;
                } else {
                    regs[instr.rd()] = a % b;
                }
            } break;

            case Opcode::REMU: {
                auto a = regs[instr.rs1()];
                auto b = regs[instr.rs2()];
                if (b == 0) {
                    regs[instr.rd()] = a;
                } else {
                    regs[instr.rd()] = (uint32_t)a % (uint32_t)b;
                }
            } break;


            case Opcode::LR_W: {
                //TODO: in multi-threaded system (or even if other components can access the memory independently, e.g. through DMA) need to mark this addr as reserved
                uint32_t addr = regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                assert (addr != 0);
                lrw_marked = addr;
            } break;

            case Opcode::SC_W: {
                uint32_t addr = regs[instr.rs1()];
                uint32_t val  = regs[instr.rs2()];
                //TODO: check if other components (besides this iss) may have accessed the last marked memory region
                if (lrw_marked == addr) {
                    mem->store_word(addr, val);
                    regs[instr.rd()] = 0;
                } else {
                    regs[instr.rd()] = 1;
                }
                lrw_marked = 0;
            } break;

            //TODO: implement the aq and rl flags if necessary (check for all AMO instructions)
            case Opcode::AMOSWAP_W: {
                uint32_t addr = regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                mem->store_word(addr, regs[instr.rs2()]);
            } break;

            case Opcode::AMOADD_W: {
                uint32_t addr = regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                uint32_t val = regs[instr.rd()] + regs[instr.rs2()];
                mem->store_word(addr, val);
            } break;

            case Opcode::AMOXOR_W: {
                uint32_t addr = regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                uint32_t val = regs[instr.rd()] ^ regs[instr.rs2()];
                mem->store_word(addr, val);
            } break;

            case Opcode::AMOAND_W: {
                uint32_t addr = regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                uint32_t val = regs[instr.rd()] & regs[instr.rs2()];
                mem->store_word(addr, val);
            } break;

            case Opcode::AMOOR_W: {
                uint32_t addr = regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                uint32_t val = regs[instr.rd()] | regs[instr.rs2()];
                mem->store_word(addr, val);
            } break;

            case Opcode::AMOMIN_W: {
                uint32_t addr = regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                uint32_t val = std::min(regs[instr.rd()], regs[instr.rs2()]);
                mem->store_word(addr, val);
            } break;

            case Opcode::AMOMINU_W: {
                uint32_t addr = regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                uint32_t val = std::min((uint32_t)regs[instr.rd()], (uint32_t)regs[instr.rs2()]);
                mem->store_word(addr, val);
            } break;

            case Opcode::AMOMAX_W: {
                uint32_t addr = regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                uint32_t val = std::max(regs[instr.rd()], regs[instr.rs2()]);
                mem->store_word(addr, val);
            } break;

            case Opcode::AMOMAXU_W: {
                uint32_t addr = regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                uint32_t val = std::max((uint32_t)regs[instr.rd()], (uint32_t)regs[instr.rs2()]);
                mem->store_word(addr, val);
            } break;


            case Opcode::WFI:
                //NOTE: only a hint, can be implemented as NOP
                //std::cout << "[sim:wfi] CSR mstatus.mie " << csrs.mstatus->mie << std::endl;
                if (!has_pending_enabled_interrupts())
                    sc_core::wait(wfi_event);
                break;

            case Opcode::SFENCE_VMA:
                //NOTE: not using MMU so far, so can be ignored
                break;

            case Opcode::URET:
            case Opcode::SRET:
                assert (false && "not implemented");
                break;
            case Opcode::MRET:
                return_from_trap_handler();
                break;

            default:
                assert (false && "unknown opcode");
        }

        //NOTE: writes to zero register are supposedly allowed but must be ignored (reset it after every instruction, instead of checking *rd != zero* before every register write)
        regs.regs[regs.zero] = 0;

        return op;
    }

    uint64_t _compute_and_get_current_cycles() {
        // Note: result is based on the default time resolution of SystemC (1 PS)
        sc_core::sc_time now = quantum_keeper.get_current_time();

        assert (now % cycle_time == sc_core::SC_ZERO_TIME);
        assert (now.value() % cycle_time.value() == 0);

        uint64_t num_cycles = now.value() / cycle_time.value();

        return num_cycles;
    }

    csr_base &csr_update_and_get(uint32_t addr) {
        switch (addr) {
            case CSR_TIME_ADDR:
            case CSR_MTIME_ADDR: {
                uint64_t mtime = clint->update_and_get_mtime();
                csrs.time_root->reg = mtime;
                return *csrs.time;
            }

            case CSR_TIMEH_ADDR:
            case CSR_MTIMEH_ADDR: {
                uint64_t mtime = clint->update_and_get_mtime();
                csrs.time_root->reg = mtime;
                return *csrs.timeh;
            }

            case CSR_MCYCLE_ADDR:
                csrs.cycle_root->reg = _compute_and_get_current_cycles();
                return *csrs.cycle;

            case CSR_MCYCLEH_ADDR:
                csrs.cycle_root->reg = _compute_and_get_current_cycles();
                return *csrs.cycleh;

            case CSR_MINSTRET_ADDR:
                return *csrs.instret;

            case CSR_MINSTRETH_ADDR:
                return *csrs.instreth;
        }

        return csrs.at(addr);
    }


    void init(instr_memory_interface *instr_mem, data_memory_interface *data_mem, clint_if *clint,
              SyscallHandler *sys, uint32_t entrypoint, uint32_t sp) {
        this->instr_mem = instr_mem;
        this->mem = data_mem;
        this->clint = clint;
        this->sys = sys;
        regs[RegFile::sp] = sp;
        pc = entrypoint;
        csrs.setup();
    }

    virtual void trigger_external_interrupt() override {
        //std::cout << "[vp::iss] trigger external interrupt" << std::endl;
        csrs.mip->meip = true;
        wfi_event.notify(sc_core::SC_ZERO_TIME);
    }

    virtual void clear_external_interrupt() override {
        csrs.mip->meip = false;
    }

    virtual void trigger_timer_interrupt(bool status) override {
        csrs.mip->mtip = status;
        wfi_event.notify(sc_core::SC_ZERO_TIME);
    }

    void return_from_trap_handler() {
        //std::cout << "[vp::iss] return from trap handler @time " << quantum_keeper.get_current_time() << " to pc " << std::hex << csrs.mepc->reg << std::endl;

        // NOTE: assumes a SW based solution to store/re-store the execution context, since this appears to be the RISC-V convention
        pc = csrs.mepc->reg;

        // NOTE: need to adapt when support for privilege levels beside M-mode is added
        csrs.mstatus->mie = csrs.mstatus->mpie;
        csrs.mstatus->mpie = 1;
    }

    bool has_pending_enabled_interrupts() {
        assert (!csrs.mip->msip && "traps and syscalls are handled in the simulator");

        return csrs.mstatus->mie && ((csrs.mie->meie && csrs.mip->meip) || (csrs.mie->mtie && csrs.mip->mtip));
    }

    void switch_to_trap_handler() {
        assert (csrs.mstatus->mie);
        //std::cout << "[vp::iss] switch to trap handler @time " << quantum_keeper.get_current_time() << " @last_pc " << std::hex << last_pc << " @pc " << pc << std::endl;

        csrs.mcause->interrupt = 1;
        if (csrs.mie->meie && csrs.mip->meip) {
            csrs.mcause->exception_code = 11;
        } else if (csrs.mie->mtie && csrs.mip->mtip) {
            csrs.mcause->exception_code = 7;
        } else {
            assert (false);     // enabled pending interrupts must be available if this function is called
        }

        // for SW traps the address of the instruction causing the trap/interrupt (i.e. last_pc, the address of the ECALL,EBREAK - better set pc=last_pc before taking trap)
        // for interrupts the address of the next instruction to execute (since e.g. the RISC-V FreeRTOS port will not modify it)
        csrs.mepc->reg = pc;

        // deactivate interrupts before jumping to trap handler (SW can re-activate if supported)
        csrs.mstatus->mpie = csrs.mstatus->mie;
        csrs.mstatus->mie = 0;

        // perform context switch to trap handler
        pc = csrs.mtvec->get_base_address();
    }


    void performance_and_sync_update(Opcode::mapping executed_op) {
        ++csrs.instret_root->reg;

        auto new_cycles = instr_cycles[executed_op];

        quantum_keeper.inc(new_cycles);
        if (quantum_keeper.need_sync()) {
            quantum_keeper.sync();
        }
    }

    void run_step() {
        assert (regs.read(0) == 0);

        //std::cout << "pc: " << std::hex << pc << " sp: " << regs.read(regs.sp) << std::endl;
        last_pc = pc;
        Opcode::mapping op = exec_step();

        if (has_pending_enabled_interrupts())
            switch_to_trap_handler();

        // Do not use a check *pc == last_pc* here. The reason is that due to interrupts *pc* can be set to *last_pc* accidentally (when jumping back to *mepc*).
        if (sys->shall_exit)
            status = CoreExecStatus::Terminated;

        // speeds up the execution performance (non debug mode) significantly by checking the additional flag first
        if (debug_mode && (breakpoints.find(pc) != breakpoints.end()))
            status = CoreExecStatus::HitBreakpoint;

        performance_and_sync_update(op);
    }

    void run() {
        // run a single step until either a breakpoint is hit or the execution terminates
        do {
            run_step();
        } while (status == CoreExecStatus::Runnable);

        // force sync to make sure that no action is missed
        quantum_keeper.sync();
    }

    void show() {
        std::cout << "simulation time: " << sc_core::sc_time_stamp() << std::endl;
        regs.show();
        std::cout << "pc = " << pc << std::endl;
        std::cout << "num-instr = " << csrs.instret_root->reg << std::endl;
        std::cout << "max-heap (c-lib malloc, bytes) = " << sys->get_max_heap_memory_consumption() << std::endl;
    }
};


/* Do not call the run function of the ISS directly but use one of the Runner wrappers. */
struct DirectCoreRunner : public sc_core::sc_module {

    ISS &core;

    SC_HAS_PROCESS(DirectCoreRunner);

    DirectCoreRunner(ISS &core)
        : sc_module(sc_core::sc_module_name("DirectCoreRunner")), core(core) {
        SC_THREAD(run);
    }

    void run() {
        core.run();

        if (core.status == CoreExecStatus::HitBreakpoint) {
            throw std::runtime_error("Breakpoints are not supported in the direct runner, use the debug runner instead.");
        }
        assert (core.status == CoreExecStatus::Terminated);

        sc_core::sc_stop();
    }
};


#endif //RISCV_ISA_ISS_H
