#ifndef RISCV_ISA_OPCODES_H
#define RISCV_ISA_OPCODES_H

namespace Opcode {
    enum parts {
        OP_LUI    = 0b0110111,
        OP_AUIPC  = 0b0010111,
        OP_JAL    = 0b1101111,
        OP_JALR   = 0b1100111,
        F3_JALR   = 0b000,  //func3

        OP_LB  = 0b0000011,
        F3_LB  = 0b000,
        F3_LH  = 0b001,
        F3_LW  = 0b010,
        F3_LBU = 0b100,
        F3_LHU = 0b101,
        //rv64

        F3_LWU = 0b110,
        F3_LD  = 0b011,

        OP_SB = 0b0100011,
        F3_SB = 0b000,
        F3_SH = 0b001,
        F3_SW = 0b010,

        //rv64
                F3_SD = 0b011,

        OP_BEQ  = 0b1100011,
        F3_BEQ  = 0b000,
        F3_BNE  = 0b001,
        F3_BLT  = 0b100,
        F3_BGE  = 0b101,
        F3_BLTU = 0b110,
        F3_BGEU = 0b111,

        OP_ADDI  = 0b0010011,
        F3_ADDI  = 0b000,
        F3_SLTI  = 0b010,
        F3_SLTIU = 0b011,
        F3_XORI  = 0b100,
        F3_ORI   = 0b110,
        F3_ANDI  = 0b111,
        F3_SLLI  = 0b001,
        F3_SRLI  = 0b101,

        //rv64 uses func6 instead of func7
        F6_SRLI  = 0b000000,
        F6_SRAI  = 0b010000,

        OP_ADD  = 0b0110011,
        F7_ADD  = 0b0000000,
        F7_SUB  = 0b0100000,
        F3_ADD  = 0b000,
        F3_SUB  = 0b000,
        F3_SLL  = 0b001,
        F3_SLT  = 0b010,
        F3_SLTU = 0b011,
        F3_XOR  = 0b100,
        F3_SRL  = 0b101,
        F3_SRA  = 0b101,
        F3_OR   = 0b110,
        F3_AND  = 0b111,

        //rv32M
                F7_MUL    = 0b0000001,
        F3_MUL    = 0b000,
        F3_MULH   = 0b001,
        F3_MULHSU = 0b010,
        F3_MULHU  = 0b011,
        F3_DIV    = 0b100,
        F3_DIVU   = 0b101,
        F3_REM    = 0b110,
        F3_REMU   = 0b111,

        //rv64
                OP_ADDIW = 0b0011011,
        F3_ADDIW = 0b000,
        F3_SLLIW = 0b001,
        F3_SRLIW = 0b101,
        //F3_SRAIW  = 0b101,
                F7_SRLIW = 0b0000000,
        F7_SRAIW = 0b0100000,

        OP_ADDW = 0b0111011,
        F7_ADDW = 0b0000000,
        F3_ADDW = 0b000,
        F3_SUBW = 0b000,
        F7_SUBW = 0b0100000,
        F3_SLLW = 0b001,
        F3_SRLW = 0b101,
        F3_SRAW = 0b101,
        //F7_SRAW   = 0b0100000, //same as SUBW

        //rv64M
                F3_MULW  = 0b000,
        F3_DIVW  = 0b100,
        F3_DIVUW = 0b101,
        F3_REMW  = 0b110,
        F3_REMUW = 0b111,
        //all rv64M instructions use the same F7
                F7_64M   = 0b0000001,

        OP_FENCE   = 0b0001111,
        OP_ECALL   = 0b1110011,
        F3_SYS     = 0b000,
        F12_ECALL  = 0b000000000000,
        F12_EBREAK = 0b000000000001,

        //begin:privileged-instructions
                F12_URET = 0b000000000010,
        F12_SRET = 0b000100000010,
        F12_MRET = 0b001100000010,
        F12_WFI  = 0b000100000101,
        F7_SFENCE_VMA = 0b0001001,
        //end:privileged-instructions

        F3_CSRRW  = 0b001,
        F3_CSRRS  = 0b010,
        F3_CSRRC  = 0b011,
        F3_CSRRWI = 0b101,
        F3_CSRRSI = 0b110,
        F3_CSRRCI = 0b111,

        //rv32A
                OP_AMO       = 0b0101111,
        F3_32A       = 0b010,
        F5_LR_W      = 0b00010,
        F5_SC_W      = 0b00011,
        F5_AMOSWAP_W = 0b00001,
        F5_AMOADD_W  = 0b00000,
        F5_AMOXOR_W  = 0b00100,
        F5_AMOAND_W  = 0b01100,
        F5_AMOOR_W   = 0b01000,
        F5_AMOMIN_W  = 0b10000,
        F5_AMOMAX_W  = 0b10100,
        F5_AMOMINU_W = 0b11000,
        F5_AMOMAXU_W = 0b11100,

        //rv64A
        //all rv64A instructions have a different F3 in comparison to rv32A
                F3_64A       = 0b011,
        F5_LR_D      = 0b00010,
        F5_SC_D      = 0b00011,
        F5_AMOSWAP_D = 0b00001,
        F5_AMOADD_D  = 0b00000,
        F5_AMOXOR_D  = 0b00100,
        F5_AMOAND_D  = 0b01100,
        F5_AMOOR_D   = 0b01000,
        F5_AMOMIN_D  = 0b10000,
        F5_AMOMAX_D  = 0b10100,
        F5_AMOMINU_D = 0b11000,
        F5_AMOMAXU_D = 0b11100,

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

        // RV64 Base Instruction Set
        LWU,
        LD,
        SD,
        //SLLI, //duplicate from RV32 - this version uses a slightly altered pattern
        //SRLI,
        //SRAI,
        ADDIW,
        SLLIW,
        SRLIW,
        SRAIW,
        ADDW,
        SUBW,
        SLLW,
        SRLW,
        SRAW,

        // RV64 M
        MULW,
        DIVW,
        DIVUW,
        REMW,
        REMUW,

        // RV64 A
        LR_D,
        SC_D,
        AMOSWAP_D,
        AMOADD_D,
        AMOXOR_D,
        AMOAND_D,
        AMOOR_D,
        AMOMIN_D,
        AMOMAX_D,
        AMOMINU_D,
        AMOMAXU_D,


        // privileged instructions
        URET,
        SRET,
        MRET,
        WFI,
        SFENCE_VMA,

        NUMBER_OF_INSTRUCTIONS
    };
}

#endif //RISCV_ISA_OPCODES_H
