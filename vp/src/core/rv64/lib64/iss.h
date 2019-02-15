#ifndef RISCV_ISA_ISS_H
#define RISCV_ISA_ISS_H

#include "stdint.h"
#include "string.h"

#include "memory.h"
#include "instr.h"
#include "bus.h"
#include "syscall.h"

#if XLEN == 64
#include "csr64.h"
#include "opcodes64.h"
#include "registers64.h"
#elif XLEN == 128
#include "csr128.h"
#include "opcodes128.h"
#include "registers128.h"
#else
#include "csr32.h"
#include "opcodes32.h"
#include "registers32.h"
#endif

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

//This option prints additional debug information

#define LOG_PC 1
#define LOG_ERROR 2
#define LOG_INSTRUCTION 4
#define LOG_JUMP 8

#define DEBUGPRINT_INSTRUCTIONS (LOG_PC | LOG_ERROR | LOG_INSTRUCTION)
// the binary representation of the current instruction for each step
//#if DEBUGPRINT_INSTRUCTIONS  //used in the debug output for unknown opcodes
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')
//#endif


struct instr_memory_interface {
    virtual ~instr_memory_interface() {}

    virtual int32_t load_instr(uint_rv_t pc) = 0; // xlen bit pc, the instruction is still 32bit for now
};


struct data_memory_interface {
    virtual ~data_memory_interface() {}

    //LOAD
    //old rv32
    virtual int_rv_t load_byte(uint_rv_t addr) = 0;

    virtual int_rv_t load_half(uint_rv_t addr) = 0;

    virtual int_rv_t load_word(uint_rv_t addr) = 0; //xlen address
    virtual uint_rv_t load_ubyte(uint_rv_t addr) = 0;

    virtual uint_rv_t load_uhalf(uint_rv_t addr) = 0;

    //new rv64
    virtual int_rv_t load_dword(uint_rv_t addr) = 0; //LD
    virtual uint_rv_t load_uword(uint_rv_t addr) = 0; //LWU

    //new rv128
    virtual int_rv_t load_qword(uint_rv_t addr) = 0; //LQ
    virtual uint_rv_t load_udword(uint_rv_t addr) = 0; //LDU


    //STORE
    //old rv32
    virtual void store_byte(uint_rv_t addr, uint8_t value) = 0;

    virtual void store_half(uint_rv_t addr, uint16_t value) = 0;

    virtual void store_word(uint_rv_t addr, uint32_t value) = 0;

    //new rv64
    virtual void store_dword(uint_rv_t addr, uint64_t value) = 0;

    //new rv128
    virtual void store_qword(uint_rv_t addr, uint128_t value) = 0;
};


struct direct_memory_interface {
    uint8_t *mem;
    uint_rv_t offset;
    uint_rv_t size;
};


struct InstrMemoryProxy : public instr_memory_interface {
    direct_memory_interface &dmi;

    tlm_utils::tlm_quantumkeeper &quantum_keeper;
    sc_core::sc_time clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
    sc_core::sc_time access_delay = clock_cycle * 2;

    InstrMemoryProxy(direct_memory_interface &dmi, tlm_utils::tlm_quantumkeeper &keeper)
            : dmi(dmi), quantum_keeper(keeper) {
    }

    virtual int32_t load_instr(uint_rv_t pc) override { //xlen bit pc (rv64)
        assert (pc >= dmi.offset);
        assert ((pc - dmi.offset) < dmi.size);

        quantum_keeper.inc(access_delay);

        return (int32_t) *((int_rv_t *) (dmi.mem + (pc - dmi.offset)));
    }
};


struct DataMemoryProxy : public data_memory_interface {
    /* Try to access the memory and redirect to the next data_memory_interface if the access is not in range */
    typedef uint_rv_t addr_t; // xlen bit addresses

    direct_memory_interface &dmi;

    data_memory_interface *next_memory;

    tlm_utils::tlm_quantumkeeper &quantum_keeper;
    sc_core::sc_time clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
    sc_core::sc_time access_delay = clock_cycle * 4;

    DataMemoryProxy(direct_memory_interface &dmi, data_memory_interface *next_memory,
                    tlm_utils::tlm_quantumkeeper &keeper)
            : dmi(dmi), next_memory(next_memory), quantum_keeper(keeper) {
    }

    template<typename T>
    inline T _load_data(addr_t addr) {
        if (addr >= dmi.offset && addr < dmi.size) {
            assert ((addr - dmi.offset + sizeof(T)) <= dmi.size);

            quantum_keeper.inc(access_delay);

            T ans = *((T *) (dmi.mem + (addr - dmi.offset)));
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
            } else if (std::is_same<T, uint64_t>::value) { //TODO new rv64
                return next_memory->load_dword(addr);
            } else {
                assert(false && "unsupported load operation");
            }
        }
    }

    template<typename T>
    inline void _store_data(addr_t addr, T value) {
        if (addr >= dmi.offset && addr < dmi.size) {
            assert ((addr - dmi.offset + sizeof(T)) <= dmi.size);

            quantum_keeper.inc(access_delay);

            *((T *) (dmi.mem + (addr - dmi.offset))) = value;
        } else {
            if (std::is_same<T, uint8_t>::value) {
                next_memory->store_byte(addr, value);
            } else if (std::is_same<T, uint16_t>::value) {
                next_memory->store_half(addr, value);
            } else if (std::is_same<T, uint32_t>::value) {
                next_memory->store_word(addr, value);
            } else if (std::is_same<T, uint64_t>::value) { //TODO rv64 store_dword
                next_memory->store_dword(addr, value);
            } else {
                assert(false && "unsupported store operation");
            }
        }
    }


    virtual int_rv_t load_byte(addr_t addr) { return _load_data<int8_t>(addr); }

    virtual int_rv_t load_half(addr_t addr) { return _load_data<int16_t>(addr); }

    virtual int_rv_t load_word(addr_t addr) { return _load_data<int32_t>(addr); }

    virtual uint_rv_t load_ubyte(addr_t addr) { return _load_data<uint8_t>(addr); }

    virtual uint_rv_t load_uhalf(addr_t addr) { return _load_data<uint16_t>(addr); }

    virtual int_rv_t load_dword(addr_t addr) { return _load_data<int64_t>(addr); }

    virtual uint_rv_t load_uword(addr_t addr) { return _load_data<uint32_t>(addr); }

    virtual int_rv_t load_qword(addr_t addr) { return _load_data<int128_t>(addr); } //rv128

    virtual uint_rv_t load_udword(addr_t addr) { return _load_data<uint64_t>(addr); } //rv128



    virtual void store_byte(addr_t addr, uint8_t value) { _store_data(addr, value); }

    virtual void store_word(addr_t addr, uint32_t value) { _store_data(addr, value); }

    virtual void store_half(addr_t addr, uint16_t value) { _store_data(addr, value); }

    virtual void store_dword(addr_t addr, uint64_t value) { _store_data(addr, value); } //rv64

    virtual void store_qword(addr_t addr, uint128_t value) { _store_data(addr, value); } //rv128
};


struct CombinedMemoryInterface : public sc_core::sc_module,
                                 public instr_memory_interface,
                                 public data_memory_interface {
    typedef uint_rv_t addr_t; //xlen address

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

    template<typename T>
    inline T _load_data(addr_t addr) {
        T ans;
        _do_transaction(tlm::TLM_READ_COMMAND, addr, (uint8_t *) &ans, sizeof(T));
        return ans;
    }

    template<typename T>
    inline void _store_data(addr_t addr, T value) {
        _do_transaction(tlm::TLM_WRITE_COMMAND, addr, (uint8_t *) &value, sizeof(T));
    }

    int32_t load_instr(addr_t addr) { return _load_data<int32_t>(addr); }

    int_rv_t load_byte(addr_t addr) { return _load_data<int8_t>(addr); }

    int_rv_t load_half(addr_t addr) { return _load_data<int16_t>(addr); }

    int_rv_t load_word(addr_t addr) { return _load_data<int32_t>(addr); }

    uint_rv_t load_ubyte(addr_t addr) { return _load_data<uint8_t>(addr); }

    uint_rv_t load_uhalf(addr_t addr) { return _load_data<uint16_t>(addr); }

    int_rv_t load_dword(addr_t addr) { return _load_data<int64_t>(addr); } //rv64
    uint_rv_t load_uword(addr_t addr) { return _load_data<uint32_t>(addr); }

    int_rv_t load_qword(addr_t addr) { return _load_data<int128_t>(addr); } // rv128
    uint_rv_t load_udword(addr_t addr) { return _load_data<uint64_t>(addr); } // rv128

    void store_byte(addr_t addr, uint8_t value) { _store_data(addr, value); }

    void store_half(addr_t addr, uint16_t value) { _store_data(addr, value); }

    void store_word(addr_t addr, uint32_t value) { _store_data(addr, value); }

    void store_dword(addr_t addr, uint64_t value) { _store_data(addr, value); } //rv64

    void store_qword(addr_t addr, uint128_t value) { _store_data(addr, value); } // rv128
};



enum class CoreExecStatus {
    Runnable,
    HitBreakpoint,
    Terminated,
};


struct ISS : public sc_core::sc_module,
             public external_interrupt_target,
             public timer_interrupt_target {

    clint_if *clint;
    instr_memory_interface *instr_mem;
    data_memory_interface *mem;
    SyscallHandler *sys;
    RegFile regs;
    uint_rv_t pc; //xlen pc
    uint_rv_t last_pc;
    csr_table csrs;

    CoreExecStatus status = CoreExecStatus::Runnable;
    std::unordered_set<uint_rv_t> breakpoints;//xlen address
    bool debug_mode = false;

    sc_core::sc_event wfi_event;

    tlm_utils::tlm_quantumkeeper quantum_keeper;
    sc_core::sc_time cycle_time;
    std::array<sc_core::sc_time, Opcode::NUMBER_OF_INSTRUCTIONS> instr_cycles;

    enum {
        REG32_MIN = INT32_MIN,
        REG64_MIN = INT64_MIN,
    };

    ISS()
            : sc_module(sc_core::sc_module_name("ISS")) {

        sc_core::sc_time qt = tlm::tlm_global_quantum::instance().get();
        cycle_time = sc_core::sc_time(10, sc_core::SC_NS);

        assert (qt >= cycle_time);
        assert (qt % cycle_time == sc_core::SC_ZERO_TIME);

        for (int i = 0; i < Opcode::NUMBER_OF_INSTRUCTIONS; ++i)
            instr_cycles[i] = cycle_time;

        const sc_core::sc_time memory_access_cycles = 4 * cycle_time;
        const sc_core::sc_time mul_div_cycles = 8 * cycle_time;

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
        //rv64
        instr_cycles[Opcode::LWU] = memory_access_cycles;
        instr_cycles[Opcode::LD] = memory_access_cycles;
        instr_cycles[Opcode::SD] = memory_access_cycles;
        instr_cycles[Opcode::MULW] = mul_div_cycles;
        instr_cycles[Opcode::DIVW] = mul_div_cycles;
        instr_cycles[Opcode::DIVUW] = mul_div_cycles;
        instr_cycles[Opcode::REMW] = mul_div_cycles;
        instr_cycles[Opcode::REMUW] = mul_div_cycles;
    }

    Opcode::mapping decode(Instruction &instr) {
        using namespace Opcode;

#if (DEBUGPRINT_INSTRUCTIONS & LOG_INSTRUCTION)
        printf("Instruction Opcode: " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(instr.opcode()));
        printf("Instruction F3: " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(instr.funct3()));
        printf("Instruction F7: " BYTE_TO_BINARY_PATTERN "\n\n", BYTE_TO_BINARY(instr.funct7()));
#endif
        switch (instr.opcode()) {  //could be split into individual extensions, which would make runtime xlen switches possible and invalid opcode detection easier
            case OP_LUI:
                return Opcode::LUI;

            case OP_AUIPC:
                return Opcode::AUIPC;

            case OP_JAL:
                return Opcode::JAL;

            case OP_JALR: {
                assert (instr.funct3() == F3_JALR);
                return Opcode::JALR;
            }

            case OP_BEQ: {
                switch (instr.funct3()) {
                    case F3_BEQ:
                        return BEQ;
                    case F3_BNE:
                        return BNE;
                    case F3_BLT:
                        return BLT;
                    case F3_BGE:
                        return BGE;
                    case F3_BLTU:
                        return BLTU;
                    case F3_BGEU:
                        return BGEU;
                }
                break;
            }

            case OP_LB: {
                switch (instr.funct3()) {
                    case F3_LB:
                        return LB;
                    case F3_LH:
                        return LH;
                    case F3_LW:
                        return LW;
                    case F3_LBU:
                        return LBU;
                    case F3_LHU:
                        return LHU;
                    case F3_LWU: //new rv64
                        return LWU;
                    case F3_LD:
                        return LD;
                }
                break;
            }

            case OP_SB: {
                switch (instr.funct3()) {
                    case F3_SB:
                        return SB;
                    case F3_SH:
                        return SH;
                    case F3_SW:
                        return SW;
                    case F3_SD: //new rv64
                        return SD;
                }
                break;
            }

            case OP_ADDI: {
                switch (instr.funct3()) {
                    case F3_ADDI:
                        return ADDI;
                    case F3_SLTI:
                        return SLTI;
                    case F3_SLTIU:
                        return SLTIU;
                    case F3_XORI:
                        return XORI;
                    case F3_ORI:
                        return ORI;
                    case F3_ANDI:
                        return ANDI;
                    case F3_SLLI:
                        return SLLI;
                    case F3_SRLI: {
#if XLEN == 64
                        switch (instr.funct6()) { //new rv64 shift instruction pattern
                            case F6_SRLI:
                                return SRLI;
                            case F6_SRAI:
                                return SRAI;
                            default:
                                printf("expected either F6:%d or F6:%d, actual value was:%d\n",
                                       F6_SRLI,F6_SRAI,instr.funct6());
                        }
#elif XLEN == 128

#else
                        switch (instr.funct7()) {
                            case F7_SRLI:
                                return SRLI;
                            case F7_SRAI:
                                return SRAI;
                            default:
                                printf("expected either F6:%d or F6:%d, actual value was:%d\n",
                                       F7_SRLI, F7_SRAI, instr.funct7());
                        }


#endif

                    }
                }
                break;
            }

            case OP_ADD: {
                switch (instr.funct7()) {
                    case F7_ADD:
                        switch (instr.funct3()) {
                            case F3_ADD:
                                return ADD;
                            case F3_SLL:
                                return SLL;
                            case F3_SLT:
                                return SLT;
                            case F3_SLTU:
                                return SLTU;
                            case F3_XOR:
                                return XOR;
                            case F3_SRL:
                                return SRL;
                            case F3_OR:
                                return OR;
                            case F3_AND:
                                return AND;
                        }
                        break;

                    case F7_SUB:
                        switch (instr.funct3()) {
                            case F3_SUB:
                                return SUB;
                            case F3_SRA:
                                return SRA;
                        }
                        break;

                    case F7_MUL:
                        switch (instr.funct3()) {
                            case F3_MUL:
                                return MUL;
                            case F3_MULH:
                                return MULH;
                            case F3_MULHSU:
                                return MULHSU;
                            case F3_MULHU:
                                return MULHU;
                            case F3_DIV:
                                return DIV;
                            case F3_DIVU:
                                return DIVU;
                            case F3_REM:
                                return REM;
                            case F3_REMU:
                                return REMU;
                        }
                        break;
                }
                break;
            }

                //new rv64
            case OP_ADDIW: {
                switch (instr.funct3()) {
                    case F3_ADDIW:
                        return ADDIW;
                    case F3_SLLIW:
                        return SLLIW;
                    case F3_SRLIW: {
                        switch (instr.funct7()) {
                            case F7_SRLIW:
                                return SRLIW;
                            case F7_SRAIW:
                                return SRAIW;
                        }
                    }
                }
                break;
            }


                //new rv64
            case OP_ADDW: {
                switch (instr.funct7()) {
                    case F7_ADDW:
                        switch (instr.funct3()) {
                            case F3_ADDW:
                                return ADDW;
                            case F3_SLLW:
                                return SLLW;
                            case F3_SRLW:
                                return SRLW;
                        }
                        break;

                    case F7_SUBW:
                        switch (instr.funct3()) {
                            case F3_SUBW:
                                return SUBW;
                            case F3_SRAW:
                                return SRAW;
                        }
                        break;

                    case F7_64M:
                        switch (instr.funct3()) {
                            case F3_MULW:
                                return MULW;
                            case F3_DIVW:
                                return DIVW;
                            case F3_DIVUW:
                                return DIVUW;
                            case F3_REMW:
                                return REMW;
                            case F3_REMUW:
                                return REMUW;
                        }
                        break;
                }
                break;
            }


            case OP_FENCE: {
                return FENCE;
            }

            case OP_ECALL: {
                switch (instr.funct3()) {
                    case F3_SYS: {
                        switch (instr.funct12()) {
                            case F12_ECALL:
                                return ECALL;
                            case F12_EBREAK:
                                return EBREAK;
                            case F12_URET:
                                return URET;
                            case F12_SRET:
                                return SRET;
                            case F12_MRET:
                                return MRET;
                            case F12_WFI:
                                return WFI;
                            default:
                                assert (instr.funct7() == F7_SFENCE_VMA && "invalid instruction detected");
                                return SFENCE_VMA;
                        }
                        break;
                    }
                    case F3_CSRRW:
                        return CSRRW;
                    case F3_CSRRS:
                        return CSRRS;
                    case F3_CSRRC:
                        return CSRRC;
                    case F3_CSRRWI:
                        return CSRRWI;
                    case F3_CSRRSI:
                        return CSRRSI;
                    case F3_CSRRCI:
                        return CSRRCI;
                }
                break;
            }

            case OP_AMO: {
                switch (instr.funct3()) {
                    case F3_32A:
                        switch (instr.funct5()) {
                            case F5_LR_W:
                                return LR_W;
                            case F5_SC_W:
                                return SC_W;
                            case F5_AMOSWAP_W:
                                return AMOSWAP_W;
                            case F5_AMOADD_W:
                                return AMOADD_W;
                            case F5_AMOXOR_W:
                                return AMOXOR_W;
                            case F5_AMOAND_W:
                                return AMOAND_W;
                            case F5_AMOOR_W:
                                return AMOOR_W;
                            case F5_AMOMIN_W:
                                return AMOMIN_W;
                            case F5_AMOMAX_W:
                                return AMOMAX_W;
                            case F5_AMOMINU_W:
                                return AMOMINU_W;
                            case F5_AMOMAXU_W:
                                return AMOMAXU_W;
                        }

                    case F3_64A:
                        switch (instr.funct5()) {
                            case F5_LR_D:
                                return LR_D;
                            case F5_SC_D:
                                return SC_D;
                            case F5_AMOSWAP_D:
                                return AMOSWAP_D;
                            case F5_AMOADD_D:
                                return AMOADD_D;
                            case F5_AMOXOR_D:
                                return AMOXOR_D;
                            case F5_AMOAND_D:
                                return AMOAND_D;
                            case F5_AMOOR_D:
                                return AMOOR_D;
                            case F5_AMOMIN_D:
                                return AMOMIN_D;
                            case F5_AMOMAX_D:
                                return AMOMAX_D;
                            case F5_AMOMINU_D:
                                return AMOMINU_D;
                            case F5_AMOMAXU_D:
                                return AMOMAXU_D;
                        }
                }
            }
        }
#if (DEBUGPRINT_INSTRUCTIONS & LOG_ERROR) //always print debug info on error (?)
        printf("------------------------------------------\n");
        printf("Error: Unknown instruction\n");

        printf("Instruction OP: " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(instr.opcode()));
        printf("Instruction F3: " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(instr.funct3()));
        printf("Instruction F7: " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(instr.funct7()));
        printf("------------------------------------------\n");
#endif
        throw std::runtime_error("unknown instruction");
    }


    Opcode::mapping exec_step() {
        auto mem_word = instr_mem->load_instr(pc);
        Instruction instr(mem_word);
        auto op = decode(instr);
        pc += 4; //TODO (C) adjust for C instructionset
        assert(pc%4==0);//TODO jumps check if pc is aligned to instruction size
        assert(regs.x0==0); //reg 0 is hardwired to 0

        switch (op) {
            case Opcode::ADDI:
                regs[instr.rd()] = regs[instr.rs1()] + instr.I_imm();
                break;

            case Opcode::SLTI:
                regs[instr.rd()] = regs[instr.rs1()] < instr.I_imm() ? 1 : 0;
                break;

            case Opcode::SLTIU:
                regs[instr.rd()] = (((uint_rv_t) regs[instr.rs1()]) < ((uint_rv_t) instr.I_imm())) ? 1 : 0;
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
                regs[instr.rd()] = ((uint_rv_t) regs[instr.rs1()]) < ((uint_rv_t) regs[instr.rs2()]);
                break;

            case Opcode::SRL:
                regs[instr.rd()] = ((uint_rv_t) regs[instr.rs1()]) >> regs.shamt(instr.rs2());
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
                regs[instr.rd()] = ((uint_rv_t) regs[instr.rs1()]) >> instr.shamt();
                break;

            case Opcode::SRAI:
                regs[instr.rd()] = regs[instr.rs1()] >> instr.shamt(); // shifts of a signed integer in C/C++ result in a arithmetic shift, logical shift for unsigned
                break;

            case Opcode::LUI:
                assert (instr.rd() != RegFile::zero);
                regs[instr.rd()] = instr.U_imm();
                break;

            case Opcode::AUIPC:
                assert (instr.rd() != RegFile::zero);
                regs[instr.rd()] = last_pc + instr.U_imm();
                break;

            case Opcode::JAL:
                if (instr.rd() != RegFile::zero)
                    regs[instr.rd()] = pc;
                pc = last_pc + instr.J_imm();
                break;

            case Opcode::JALR: {
                uint_rv_t link = pc; //xlen pc
                pc = (regs[instr.rs1()] + instr.I_imm()) & ~1;
#if (DEBUGPRINT_INSTRUCTIONS & LOG_JUMP)
                printf("jump to:%x+%x=%x\n",regs[instr.rs1()],instr.I_imm(),pc);
#endif
                if (instr.rd() != RegFile::zero)
                    regs[instr.rd()] = link;
            }
                break;

            case Opcode::SB: {
                uint_rv_t addr = regs[instr.rs1()] + instr.S_imm(); //xlen addresses
                mem->store_byte(addr, regs[instr.rs2()]);
            }
                break;

            case Opcode::SH: {
                uint_rv_t addr = regs[instr.rs1()] + instr.S_imm();
                mem->store_half(addr, regs[instr.rs2()]);
            }
                break;

            case Opcode::SW: {
                uint_rv_t addr = regs[instr.rs1()] + instr.S_imm();
                mem->store_word(addr, regs[instr.rs2()]);
            }
                break;

            case Opcode::LB: {
                uint_rv_t addr = regs[instr.rs1()] + instr.I_imm();
                regs[instr.rd()] = mem->load_byte(addr);
            }
                break;

            case Opcode::LH: {
                uint_rv_t addr = regs[instr.rs1()] + instr.I_imm();
                regs[instr.rd()] = mem->load_half(addr);
            }
                break;

            case Opcode::LW: {
                uint_rv_t addr = regs[instr.rs1()] + instr.I_imm();
                regs[instr.rd()] = mem->load_word(addr);
            }
                break;

            case Opcode::LBU: {
                uint_rv_t addr = regs[instr.rs1()] + instr.I_imm();
                regs[instr.rd()] = mem->load_ubyte(addr);
            }
                break;

            case Opcode::LHU: {
                uint_rv_t addr = regs[instr.rs1()] + instr.I_imm();
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
                if ((uint_rv_t) regs[instr.rs1()] < (uint_rv_t) regs[instr.rs2()])
                    pc = last_pc + instr.B_imm();
                break;

            case Opcode::BGEU:
                if ((uint_rv_t) regs[instr.rs1()] >= (uint_rv_t) regs[instr.rs2()])
                    pc = last_pc + instr.B_imm();
                break;

            case Opcode::FENCE: {
                // not using out of order execution so can be ignored
                break;
            }

            case Opcode::ECALL: {
                // NOTE: cast to unsigned value to avoid sign extension, since execute_syscall expects a native 64 bit value //TODO not neccessary anymore for rv64
                int ans = sys->execute_syscall((uint_rv_t) regs[RegFile::a7], (uint_rv_t) regs[RegFile::a0],
                                               (uint_rv_t) regs[RegFile::a1], (uint_rv_t) regs[RegFile::a2],
                                               (uint_rv_t) regs[RegFile::a3]);
                regs[RegFile::a0] = ans;
            }
                break;

            case Opcode::EBREAK:
                status = CoreExecStatus::HitBreakpoint;
                break;

            case Opcode::CSRRW: {
                auto rd = instr.rd();
                auto &csr = csr_update_and_get(instr.csr());
                if (rd != RegFile::zero) {
                    regs[instr.rd()] = csr.read();
                }
                csr.write(regs[instr.rs1()]);
            }
                break;

            case Opcode::CSRRS: {
                auto rd = instr.rd();
                auto rs1 = instr.rs1();
                auto &csr = csr_update_and_get(instr.csr());
                if (rd != RegFile::zero)
                    regs[rd] = csr.read();
                if (rs1 != RegFile::zero) {
                    csr.set_bits(regs[rs1]);
                }
            }
                break;

            case Opcode::CSRRC: {
                auto rd = instr.rd();
                auto rs1 = instr.rs1();
                auto &csr = csr_update_and_get(instr.csr());
                if (rd != RegFile::zero)
                    regs[rd] = csr.read();
                if (rs1 != RegFile::zero) {
                    csr.clear_bits(regs[rs1]);
                }
            }
                break;

            case Opcode::CSRRWI: {
                auto rd = instr.rd();
                auto &csr = csr_update_and_get(instr.csr());
                if (rd != RegFile::zero) {
                    regs[rd] = csr.read();
                }
                csr.write(instr.zimm());
            }
                break;

            case Opcode::CSRRSI: {
                auto rd = instr.rd();
                auto zimm = instr.zimm();
                auto &csr = csr_update_and_get(instr.csr());
                if (rd != RegFile::zero)
                    regs[rd] = csr.read();
                if (zimm != 0) {
                    csr.set_bits(zimm);
                }
            }
                break;

            case Opcode::CSRRCI: {
                auto rd = instr.rd();
                auto zimm = instr.zimm();
                auto &csr = csr_update_and_get(instr.csr());
                if (rd != RegFile::zero)
                    regs[rd] = csr.read();
                if (zimm != 0) {
                    csr.clear_bits(zimm);
                }
            }
                break;


            case Opcode::MUL: {
                regs[instr.rd()] = (int_rv_next_t) regs[instr.rs1()] * (int_rv_next_t) regs[instr.rs2()];
            }
                break;

            case Opcode::MULH: {
                regs[instr.rd()] = (int_rv_next_t) regs[instr.rs1()] * (int_rv_next_t) regs[instr.rs2()];
            }
                break;

            case Opcode::MULHU: {
                regs[instr.rd()] = (uint_rv_next_t) regs[instr.rs1()] * (uint_rv_next_t) regs[instr.rs2()];
            }
                break;

            case Opcode::MULHSU: {
                regs[instr.rd()] = (int_rv_next_t) regs[instr.rs1()] * (uint_rv_next_t) regs[instr.rs2()];
            }
                break;

            case Opcode::DIV: {
                auto a = regs[instr.rs1()];
                auto b = regs[instr.rs2()];
                if (b == 0) {
                    regs[instr.rd()] = -1;
                } else if (a == REG64_MIN && b == -1) {
                    regs[instr.rd()] = a;
                } else {
                    regs[instr.rd()] = a / b;
                }
            }
                break;

            case Opcode::DIVU: {
                auto a = regs[instr.rs1()];
                auto b = regs[instr.rs2()];
                if (b == 0) {
                    regs[instr.rd()] = -1;
                } else {
                    regs[instr.rd()] = (uint_rv_t) a / (uint_rv_t) b;
                }
            }
                break;

            case Opcode::REM: {
                auto a = regs[instr.rs1()];
                auto b = regs[instr.rs2()];
                if (b == 0) {
                    regs[instr.rd()] = a;
                } else if (a == REG32_MIN && b == -1) {
                    regs[instr.rd()] = 0;
                } else {
                    regs[instr.rd()] = a % b;
                }
            }
                break;

            case Opcode::REMU: {
                auto a = regs[instr.rs1()];
                auto b = regs[instr.rs2()];
                if (b == 0) {
                    regs[instr.rd()] = a;
                } else {
                    regs[instr.rd()] = (uint_rv_t) a % (uint_rv_t) b;
                }
            }
                break;


            case Opcode::LR_W: {
                //TODO: in multi-threaded system (or even if other components can access the memory independently, e.g. through DMA) need to mark this addr as reserved
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()]; //xlen address
                regs[instr.rd()] = mem->load_word(addr);
            }
                break;

            case Opcode::SC_W: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                uint32_t val = (uint32_t) regs[instr.rs2()];
                mem->store_word(addr, val);
                regs[instr.rd()] = 0;       //TODO: assume success for now (i.e. the last marked word with LR_W has not been accessed in-between)
            }
                break;

                //TODO: implement the aq and rl flags if necessary (check for all AMO instructions)
            case Opcode::AMOSWAP_W: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                int32_t val = (int32_t) mem->load_word(addr);
                regs[instr.rd()] = val;
                mem->store_word(addr, val);
            }
                break;

            case Opcode::AMOADD_W: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                int32_t val = (int32_t) regs[instr.rd()] + (int32_t) regs[instr.rs2()];
                mem->store_word(addr, val);
            }
                break;

            case Opcode::AMOXOR_W: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                uint_rv_t val = (uint32_t) regs[instr.rd()] ^(uint32_t) regs[instr.rs2()];
                mem->store_word(addr, val);
            }
                break;

            case Opcode::AMOAND_W: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                uint_rv_t val = (uint32_t) regs[instr.rd()] & (uint32_t) regs[instr.rs2()];
                mem->store_word(addr, val);
            }
                break;

            case Opcode::AMOOR_W: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                uint_rv_t val = (uint32_t) regs[instr.rd()] | (uint32_t) regs[instr.rs2()];
                mem->store_word(addr, val);
            }
                break;

            case Opcode::AMOMIN_W: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                uint_rv_t val = (uint_rv_t) std::min((int32_t) regs[instr.rd()], (int32_t) regs[instr.rs2()]);
                mem->store_word(addr, val);
            }
                break;

            case Opcode::AMOMINU_W: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                uint_rv_t val = std::min((uint32_t) regs[instr.rd()], (uint32_t) regs[instr.rs2()]);
                mem->store_word(addr, val);
            }
                break;

            case Opcode::AMOMAX_W: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                uint_rv_t val = (uint_rv_t) std::max((int32_t) regs[instr.rd()], (int32_t) regs[instr.rs2()]);
                mem->store_word(addr, val);
            }
                break;

            case Opcode::AMOMAXU_W: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_word(addr);
                uint_rv_t val = std::max((uint32_t) regs[instr.rd()], (uint32_t) regs[instr.rs2()]);
                mem->store_word(addr, val);
            }
                break;


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
            case Opcode::MRET:
                return_from_trap_handler();
                break;

                //RV64 Base
            case Opcode::LWU: {
                uint_rv_t addr = regs[instr.rs1()] + instr.I_imm();
                regs[instr.rd()] = mem->load_uword(addr);
            }
                break;

            case Opcode::LD: {
                uint_rv_t addr = regs[instr.rs1()] + instr.I_imm();
                regs[instr.rd()] = mem->load_dword(addr);
                printf("LOAD_DATA: " BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(regs[instr.rd()]));
            }
                break;


            case Opcode::SD: {
                uint_rv_t addr = regs[instr.rs1()] + instr.S_imm();
                mem->store_dword(addr, regs[instr.rs2()]);
            }
                break;


            case Opcode::ADDIW: {
                regs[instr.rd()] = ((int32_t) regs[instr.rs1()] + (int32_t) instr.I_imm());
            }
                break;

            case Opcode::SLLIW: {
                regs[instr.rd()] = ((uint32_t) regs[instr.rs1()] << instr.shamt());
            }
                break;

            case Opcode::SRLIW: {
                regs[instr.rd()] = (((uint32_t) regs[instr.rs1()]) >> instr.shamt());
            }
                break;

            case Opcode::SRAIW: {
                regs[instr.rd()] = ((int32_t) regs[instr.rs1()] >> instr.shamt());
            }
                break;

            case Opcode::ADDW: {
                regs[instr.rd()] = ((int32_t) regs[instr.rs1()] + (int32_t) regs[instr.rs2()]);
            }
                break;

            case Opcode::SUBW: {
                regs[instr.rd()] = ((int32_t) regs[instr.rs1()] - (int32_t) regs[instr.rs2()]);
            }
                break;

            case Opcode::SLLW: {
                regs[instr.rd()] = ((uint32_t) regs[instr.rs1()] << regs.shamt(instr.rs2()));
            }
                break;

            case Opcode::SRLW: {
                regs[instr.rd()] = ((uint32_t) regs[instr.rs1()] >> regs.shamt(instr.rs2()));
            }
                break;

            case Opcode::SRAW: {
                regs[instr.rd()] = ((int32_t) regs[instr.rs1()] >> regs.shamt(instr.rs2()));
            }
                break;


                //RV64 M
            case Opcode::MULW: {  //calculate the value using the usual xlen, but cast to sign extended 32bit
                regs[instr.rd()] = ((int32_t) ((int64_t) regs[instr.rs1()] * (int64_t) regs[instr.rs2()]));
            }
                break;

            case Opcode::DIVW: {
                int32_t a = (int32_t) regs[instr.rs1()];
                int32_t b = (int32_t) regs[instr.rs2()];
                if (b == 0) {
                    regs[instr.rd()] = -1; //int32?
                } else if (a == REG32_MIN && b == -1) {//might have to change regmin
                    regs[instr.rd()] = a;
                } else {
                    regs[instr.rd()] = a / b;
                }
            }
                break;

            case Opcode::DIVUW: {
                int32_t a = (int32_t) regs[instr.rs1()];
                int32_t b = (int32_t) regs[instr.rs2()];
                if (b == 0) {
                    regs[instr.rd()] = -1;
                } else {
                    regs[instr.rd()] = (uint32_t) a / (uint32_t) b;
                }
            }
                break;

            case Opcode::REMW: {
                int32_t a = (int32_t) regs[instr.rs1()];
                int32_t b = (int32_t) regs[instr.rs2()];
                if (b == 0) {
                    regs[instr.rd()] = a;
                } else if (a == REG32_MIN && b == -1) {
                    regs[instr.rd()] = 0;
                } else {
                    regs[instr.rd()] = a % b;
                }
            }
                break;

            case Opcode::REMUW: {
                int32_t a = (int32_t) regs[instr.rs1()];
                int32_t b = (int32_t) regs[instr.rs2()];
                if (b == 0) {
                    regs[instr.rd()] = a;
                } else {
                    regs[instr.rd()] = (uint32_t) a % (uint32_t) b;
                }
            }
                break;



                //RV64 A

            case Opcode::LR_D: {//see LRW
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_dword(addr);
            }
                break;

            case Opcode::SC_D: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                uint_rv_t val = (uint_rv_t) regs[instr.rs2()];
                mem->store_dword(addr, val);
                regs[instr.rd()] = 0;//see SCW
            }
                break;

            case Opcode::AMOSWAP_D: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_dword(addr);
                mem->store_dword(addr, (uint_rv_t) regs[instr.rs2()]);
            }
                break;

            case Opcode::AMOADD_D: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_dword(addr);
                uint_rv_t val = (uint_rv_t) (regs[instr.rd()] + regs[instr.rs2()]);
                mem->store_dword(addr, val);
            }
                break;

            case Opcode::AMOXOR_D: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_dword(addr);
                uint_rv_t val = (uint_rv_t) (regs[instr.rd()] ^ regs[instr.rs2()]);
                mem->store_dword(addr, val);
            }
                break;

            case Opcode::AMOAND_D: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_dword(addr);
                uint_rv_t val = (uint_rv_t) (regs[instr.rd()] & regs[instr.rs2()]);
                mem->store_dword(addr, val);
            }
                break;

            case Opcode::AMOOR_D: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_dword(addr);
                uint_rv_t val = (uint_rv_t) (regs[instr.rd()] | regs[instr.rs2()]);
                mem->store_dword(addr, val);
            }
                break;

            case Opcode::AMOMIN_D: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_dword(addr);
                uint_rv_t val = (uint_rv_t) std::min(regs[instr.rd()], regs[instr.rs2()]);
                mem->store_dword(addr, val);
            }
                break;

            case Opcode::AMOMAX_D: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_dword(addr);
                uint_rv_t val = (uint_rv_t) std::max(regs[instr.rd()], regs[instr.rs2()]);
                mem->store_dword(addr, val);
            }
                break;

            case Opcode::AMOMINU_D: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_dword(addr);
                uint_rv_t val = std::min((uint_rv_t) regs[instr.rd()], (uint_rv_t) regs[instr.rs2()]);
                mem->store_dword(addr, val);
            }
                break;

            case Opcode::AMOMAXU_D: {
                uint_rv_t addr = (uint_rv_t) regs[instr.rs1()];
                regs[instr.rd()] = mem->load_dword(addr);
                uint_rv_t val = std::max((uint_rv_t) regs[instr.rd()], (uint_rv_t) regs[instr.rs2()]);
                mem->store_dword(addr, val);
            }
                break;

            default:
                assert (false && "unknown opcode");
        }

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

#if XLEN == 64
    csr_base &csr_update_and_get(uint32_t addr) {//actually a 12bit addressspace
        switch (addr) {
            case CSR_TIME_ADDR:
            case CSR_MTIME_ADDR: {
                uint64_t mtime = clint->update_and_get_mtime();
                csrs.time->reg = mtime;
                return *csrs.time;
            }

            case CSR_TIMEH_ADDR:
            case CSR_MTIMEH_ADDR: {
                assert(false && "Illegal Instruction for RV64");
            }

            case CSR_MCYCLE_ADDR:
                csrs.cycle->reg = _compute_and_get_current_cycles();
                return *csrs.cycle;

            case  CSR_MCYCLEH_ADDR:
                assert(false && "Illegal Instruction for RV64");

            case CSR_MINSTRET_ADDR:
                return *csrs.instret;

            case CSR_MINSTRETH_ADDR:
                assert(false && "Illegal Instruction for RV64");

                //new
            case CSR_INSTRET_ADDR:
                return *csrs.instret;
        }

        return csrs.at(addr);
    }
#elif XLEN == 128
    //TODO 128 bit

#else
    csr_base &csr_update_and_get(uint_rv_t addr) {//actually a 12bit addressspace
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
#endif

    void init(instr_memory_interface *instr_mem, data_memory_interface *data_mem, clint_if *clint,
              SyscallHandler *sys, uint_rv_t entrypoint, uint_rv_t sp) { //xlen
        this->instr_mem = instr_mem;
        this->mem = data_mem;
        this->clint = clint;
        this->sys = sys;
        regs[RegFile::sp] = sp;
        pc = entrypoint;
        csrs.setup();
    }

    virtual void trigger_external_interrupt() override {
        //std::cout << "[sim] trigger external interrupt" << std::endl;
        csrs.mip->meip = true;
        wfi_event.notify(sc_core::SC_ZERO_TIME);
    }

    virtual void trigger_timer_interrupt(bool status) override {
        csrs.mip->mtip = status;
        wfi_event.notify(sc_core::SC_ZERO_TIME);
    }

    void return_from_trap_handler() {
        //std::cout << "[sim] return from trap handler @time " << quantum_keeper.get_current_time() << " to pc " << std::hex << csrs.mepc->reg << std::endl;

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
        //std::cout << "[sim] switch to trap handler @time " << quantum_keeper.get_current_time() << " @last_pc " << std::hex << last_pc << " @pc " << pc << std::endl;

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
#if XLEN == 64
        ++csrs.instret->reg;

#elif XLEN == 128
        ++csrs.instret->reg;
#else
        ++csrs.instret_root->reg;
#endif
        auto new_cycles = instr_cycles[executed_op];

        quantum_keeper.inc(new_cycles);
        if (quantum_keeper.need_sync()) {
            quantum_keeper.sync();
        }
    }

    void run_step() {
        assert (regs.regs[0] == 0);
#if (DEBUGPRINT_INSTRUCTIONS & LOG_PC)
        std::cout << "pc: " << std::hex << pc << " sp: " << regs.regs[regs.sp] << std::dec << std::endl;
#endif
        last_pc = pc;
        Opcode::mapping op = exec_step();

        if (has_pending_enabled_interrupts())
            switch_to_trap_handler();

        // Do not use a check *pc == last_pc* here. The reason is that due to interrupts *pc* can be set to *last_pc* accidentally (when jumping back to *mepc*).
        if (sys->shall_exit) //TODO fix shall exit to work correctly for 64ELFs
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
#if XLEN == 64 //TODO cleanup
        std::cout << "num-instr = " << csrs.instret->reg << std::endl;
#elif XLEN == 128
        std::cout << "num-instr = " << csrs.instret->reg << std::endl;
#else
        std::cout << "num-instr = " << csrs.instret_root->reg << std::endl;
#endif
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
            throw std::runtime_error(
                    "Breakpoints are not supported in the direct runner, use the debug runner instead.");
        }
        assert (core.status == CoreExecStatus::Terminated);

        sc_core::sc_stop();
    }
};


#endif //RISCV_ISA_ISS_H
