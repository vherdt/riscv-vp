#ifndef RISCV_ISA_INSTR_H
#define RISCV_ISA_INSTR_H

#include "stdint.h"


namespace Opcode {
    enum parts {
        OP_LUI    = 0b0110111,
        OP_AUIPC  = 0b0010111,
        OP_JAL    = 0b1101111,
        OP_JALR   = 0b1100111,
        F3_JALR   = 0b000,

        OP_LB     = 0b0000011,
        F3_LB     = 0b000,
        F3_LH     = 0b001,
        F3_LW     = 0b010,
        F3_LBU    = 0b100,
        F3_LHU    = 0b101,

        OP_SB     = 0b0100011,
        F3_SB     = 0b000,
        F3_SH     = 0b001,
        F3_SW     = 0b010,

        OP_BEQ    = 0b1100011,
        F3_BEQ    = 0b000,
        F3_BNE    = 0b001,
        F3_BLT    = 0b100,
        F3_BGE    = 0b101,
        F3_BLTU   = 0b110,
        F3_BGEU   = 0b111,

        OP_ADDI   = 0b0010011,
        F3_ADDI   = 0b000,
        F3_SLTI   = 0b010,
        F3_SLTIU  = 0b011,
        F3_XORI   = 0b100,
        F3_ORI    = 0b110,
        F3_ANDI   = 0b111,
        F3_SLLI   = 0b001,
        F3_SRLI   = 0b101,
        F7_SRLI   = 0b0000000,
        F7_SRAI   = 0b0100000,

        OP_ADD    = 0b0110011,
        F7_ADD    = 0b0000000,
        F7_SUB    = 0b0100000,
        F3_ADD    = 0b000,
        F3_SUB    = 0b000,
        F3_SLL    = 0b001,
        F3_SLT    = 0b010,
        F3_SLTU   = 0b011,
        F3_XOR    = 0b100,
        F3_SRL    = 0b101,
        F3_SRA    = 0b101,
        F3_OR     = 0b110,
        F3_AND    = 0b111,

        F7_MUL    = 0b0000001,
        F3_MUL    = 0b000,
        F3_MULH   = 0b001,
        F3_MULHSU = 0b010,
        F3_MULHU  = 0b011,
        F3_DIV    = 0b100,
        F3_DIVU   = 0b101,
        F3_REM    = 0b110,
        F3_REMU   = 0b111,

        OP_FENCE   = 0b0001111,
        OP_ECALL   = 0b1110011,
        F3_SYS     = 0b000,
        F12_ECALL  = 0b000000000000,
        F12_EBREAK = 0b000000000001,
        //begin:privileged-instructions
                F12_URET   = 0b000000000010,
        F12_SRET   = 0b000100000010,
        F12_MRET   = 0b001100000010,
        F12_WFI    = 0b000100000101,
        F7_SFENCE_VMA = 0b0001001,
        //end:privileged-instructions
                F3_CSRRW   = 0b001,
        F3_CSRRS   = 0b010,
        F3_CSRRC   = 0b011,
        F3_CSRRWI  = 0b101,
        F3_CSRRSI  = 0b110,
        F3_CSRRCI  = 0b111,

        OP_AMO        = 0b0101111,
        F5_LR_W       = 0b00010,
        F5_SC_W       = 0b00011,
        F5_AMOSWAP_W  = 0b00001,
        F5_AMOADD_W   = 0b00000,
        F5_AMOXOR_W   = 0b00100,
        F5_AMOAND_W   = 0b01100,
        F5_AMOOR_W    = 0b01000,
        F5_AMOMIN_W   = 0b10000,
        F5_AMOMAX_W   = 0b10100,
        F5_AMOMINU_W  = 0b11000,
        F5_AMOMAXU_W  = 0b11100,
    };

    enum mapping {
        UNDEF = 0,

        // RV32I Base Instruction Set
        LUI = 1,
        AUIPC,
        JAL,
        JALR,
        BEQ,
        BNE,
        BLT,
        BGE,
        BLTU,
        BGEU,
        LB,
        LH,
        LW,
        LBU,
        LHU,
        SB,
        SH,
        SW,
        ADDI,
        SLTI,
        SLTIU,
        XORI,
        ORI,
        ANDI,
        SLLI,
        SRLI,
        SRAI,
        ADD,
        SUB,
        SLL,
        SLT,
        SLTU,
        XOR,
        SRL,
        SRA,
        OR,
        AND,
        FENCE,
        ECALL,
        EBREAK,
        CSRRW,
        CSRRS,
        CSRRC,
        CSRRWI,
        CSRRSI,
        CSRRCI,

        // RV32M Standard Extension
        MUL,
        MULH,
        MULHSU,
        MULHU,
        DIV,
        DIVU,
        REM,
        REMU,

        // RV32A Standard Extension
        LR_W,
        SC_W,
        AMOSWAP_W,
        AMOADD_W,
        AMOXOR_W,
        AMOAND_W,
        AMOOR_W,
        AMOMIN_W,
        AMOMAX_W,
        AMOMINU_W,
        AMOMAXU_W,

        // privileged instructions
        URET,
        SRET,
        MRET,
        WFI,
        SFENCE_VMA,

        NUMBER_OF_INSTRUCTIONS
    };
}


#define BIT_RANGE(instr,upper,lower) (instr & (((1 << (upper-lower+1)) - 1) << lower))
#define BIT_SLICE(instr,upper,lower) (BIT_RANGE(instr,upper,lower) >> lower)
#define BIT_SINGLE(instr,pos) (instr & (1 << pos))
#define BIT_SINGLE_P1(instr,pos) (BIT_SINGLE(instr,pos) >> pos)
#define BIT_SINGLE_PN(instr,pos,new_pos) ((BIT_SINGLE(instr,pos) >> pos) << new_pos)
#define EXTRACT_SIGN_BIT(instr,pos,new_pos) ((BIT_SINGLE_P1(instr,pos) << 31) >> (31 - new_pos))


struct Instruction {
    Instruction(uint32_t instr)
            : instr(instr) {
    }

    inline uint32_t quadrant() {
        return instr & 0x3;
    }

    inline bool is_compressed() {
        return quadrant() < 3;
    }

    inline uint32_t c_format() {
        return instr & 0xffff;
    }

    inline uint32_t c_opcode() {
        return BIT_SLICE(instr, 15, 13);
    }

    inline uint32_t c_b12() {
        return BIT_SINGLE_P1(instr, 12);
    }

    inline uint32_t c_rd() {
        return rd();
    }

    inline uint32_t c_rd_small() {
        return BIT_SLICE(instr, 9, 7) | 8;
    }

    inline uint32_t c_rs2_small() {
        return BIT_SLICE(instr, 4, 2) | 8;
    }

    inline uint32_t c_rs2() {
        return BIT_SLICE(instr, 6, 2);
    }

    inline uint32_t c_imm() {
        return BIT_SLICE(instr,6,2) | EXTRACT_SIGN_BIT(instr,12,5);
    }

    inline uint32_t c_uimm() {
        return BIT_SLICE(instr,6,2) | (BIT_SINGLE_P1(instr,12) << 5);
    }

    inline uint32_t c_f2_high() {
        return BIT_SLICE(instr,11,10);
    }

    inline uint32_t c_f2_low() {
        return BIT_SLICE(instr,6,5);
    }

    Opcode::mapping decode_normal();

    Opcode::mapping decode_and_expand_compressed();

    inline uint32_t csr() {
        // cast to unsigned to avoid sign extension when shifting
        return BIT_RANGE((uint32_t)instr, 31, 20) >> 20;
    }

    inline uint32_t zimm() {
        return BIT_RANGE(instr, 19, 15) >> 15;
    }

    inline uint32_t shamt() {
        return (BIT_RANGE(instr, 24, 20) >> 20);
    }

    inline int32_t funct3() {
        return (BIT_RANGE(instr, 14, 12) >> 12);
    }

    inline int32_t funct12() {
        // cast to unsigned to avoid sign extension when shifting
        return (BIT_RANGE((uint32_t)instr, 31, 20) >> 20);
    }

    inline int32_t funct7() {
        // cast to unsigned to avoid sign extension when shifting
        return (BIT_RANGE((uint32_t)instr, 31, 25) >> 25);
    }

    inline int32_t funct5() {
        // cast to unsigned to avoid sign extension when shifting
        return (BIT_RANGE((uint32_t)instr, 31, 27) >> 27);
    }

    inline bool aq() {
        return BIT_SINGLE(instr, 26);
    }

    inline bool rl() {
        return BIT_SINGLE(instr, 25);
    }

    inline int32_t opcode() {
        return BIT_RANGE(instr, 6, 0);
    }


    inline int32_t J_imm() {
        return (BIT_SINGLE(instr,31) >> 11) | BIT_RANGE(instr,19,12) | (BIT_SINGLE(instr,20) >> 9) | (BIT_RANGE(instr,30,21) >> 20);
    }

    inline int32_t I_imm() {
        return BIT_RANGE(instr,31,20) >> 20;
    }

    inline int32_t S_imm() {
        return (BIT_RANGE(instr,31,25) >> 20) | (BIT_RANGE(instr,11,7) >> 7);
    }

    inline int32_t B_imm() {
        return (BIT_SINGLE(instr,31) >> 19) | (BIT_SINGLE(instr,7) << 4) | (BIT_RANGE(instr,30,25) >> 20) | (BIT_RANGE(instr,11,8) >> 7);
    }

    inline int32_t U_imm() {
        return BIT_RANGE(instr,31,12);
    }


    inline uint32_t rs1() {
        return BIT_RANGE(instr,19,15) >> 15;
    }

    inline uint32_t rs2() {
        return BIT_RANGE(instr,24,20) >> 20;
    }

    inline uint32_t rd() {
        return BIT_RANGE(instr,11,7) >> 7;
    }

    inline uint32_t data() {
        return instr;
    }

private:
    // use signed variable to have correct sign extension in immediates
    int32_t instr;
};


#endif //RISCV_ISA_INSTR_H
