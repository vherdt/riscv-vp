#include "instr.h"

#include <stdexcept>
#include <cassert>

namespace Compressed {
    enum Opcode {
        // quadrant zero
        C_Illegal,
        C_Reserved,
        C_ADDI4SPN,
        C_LW,
        C_SW,

        // quadrant one
        C_NOP,
        C_ADDI,
        C_JAL,
        C_LI,
        C_ADDI16SP,
        C_LUI,
        C_SRLI,
        C_SRAI,
        C_ANDI,
        C_SUB,
        C_XOR,
        C_OR,
        C_AND,
        C_J,
        C_BEQZ,
        C_BNEZ,

        // quadrant two
        C_SLLI,
        C_LWSP,
        C_JR,
        C_MV,
        C_EBREAK,
        C_JALR,
        C_ADD,
        C_SWSP,
    };
}


unsigned C_ADDI4SPN_NZUIMM(uint32_t n) {
     return (BIT_SLICE(n,12,11) << 4) | (BIT_SLICE(n,10,7) << 6) | (BIT_SINGLE_P1(n,6) << 2) | (BIT_SINGLE_P1(n,5) << 3);
}

unsigned C_LW_UIMM(uint32_t n) {
    return (BIT_SLICE(n,12,10) << 3) | (BIT_SINGLE_P1(n,6) << 2) | (BIT_SINGLE_P1(n,5) << 6);
}

unsigned C_SW_UIMM(uint32_t n) {
    return C_LW_UIMM(n);
}

int32_t C_JAL_IMM(int32_t n) {
    return EXTRACT_SIGN_BIT(n,12,11) | BIT_SINGLE_PN(n,11,4) | (BIT_SLICE(n,10,9) << 8) | BIT_SINGLE_PN(n,8,10) | BIT_SINGLE_PN(n,7,6) | BIT_SINGLE_PN(n,6,7) | (BIT_SLICE(n,5,3) << 1) | BIT_SINGLE_PN(n,2,5);
}

int32_t C_ADDI16SP_NZIMM(int32_t n) {
    return EXTRACT_SIGN_BIT(n,12,9) | BIT_SINGLE_PN(n,6,4) | BIT_SINGLE_PN(n,5,6) | (BIT_SLICE(n,4,3) << 7) | BIT_SINGLE_PN(n,2,5);
}

int32_t C_LUI_NZIMM(int32_t n) {
    return EXTRACT_SIGN_BIT(n,12,17) | (BIT_SLICE(n,6,2) << 12);
}

int32_t C_J_IMM(int32_t n) {
    return C_JAL_IMM(n);
}

int32_t C_BRANCH_IMM(int32_t n) {
    return EXTRACT_SIGN_BIT(n,12,8) | (BIT_SLICE(n,11,10) << 3) | (BIT_SLICE(n,6,5) << 6) | (BIT_SLICE(n,4,3) << 1) | BIT_SINGLE_PN(n,2,5);
}

uint32_t C_LWSP_UIMM(uint32_t n) {
    return BIT_SINGLE_PN(n,12,5) | (BIT_SLICE(n,6,4) << 2) | (BIT_SLICE(n,3,2) << 6);
}

uint32_t C_SWSP_UIMM(uint32_t n) {
    return (BIT_SLICE(n,12,9) << 2) | (BIT_SLICE(n,8,7) << 6);
}



struct InstructionFactory {
    typedef Instruction T;

    static T ADD(unsigned rd, unsigned rs1, unsigned rs2) {
        return T(((rd & 0x1f) << 7) | ((rs1 & 0x1f) << 15) | ((rs2 & 0x1f) << 20) | 51 | (0 << 12) | (0 << 25));
    }

    static T AND(unsigned rd, unsigned rs1, unsigned rs2) {
        return T(((rd & 0x1f) << 7) | ((rs1 & 0x1f) << 15) | ((rs2 & 0x1f) << 20) | 51 | (7 << 12) | (0 << 25));
    }

    static T OR(unsigned rd, unsigned rs1, unsigned rs2) {
        return T(((rd & 0x1f) << 7) | ((rs1 & 0x1f) << 15) | ((rs2 & 0x1f) << 20) | 51 | (6 << 12) | (0 << 25));
    }

    static T XOR(unsigned rd, unsigned rs1, unsigned rs2) {
        return T(((rd & 0x1f) << 7) | ((rs1 & 0x1f) << 15) | ((rs2 & 0x1f) << 20) | 51 | (4 << 12) | (0 << 25));
    }

    static T SUB(unsigned rd, unsigned rs1, unsigned rs2) {
        return T(((rd & 0x1f) << 7) | ((rs1 & 0x1f) << 15) | ((rs2 & 0x1f) << 20) | 51 | (0 << 12) | (32 << 25));
    }

    static T LW(unsigned rd, unsigned rs1, int I_imm) {
        return T(((I_imm & 4095) << 20) | ((rd & 0x1f) << 7) | ((rs1 & 0x1f) << 15) | 3 | (2 << 12));
    }

    static T SW(unsigned rs1, unsigned rs2, int S_imm) {
        return T((((S_imm & 0b11111) << 7) | ((S_imm & (0b1111111 << 5)) << 20)) | ((rs1 & 0x1f) << 15) | ((rs2 & 0x1f) << 20) | 35 | (2 << 12));
    }

    static T LUI(unsigned rd, int U_imm) {
        return T((U_imm & (1048575 << 12)) | ((rd & 0x1f) << 7) | 55);
    }

    static T ADDI(unsigned rd, unsigned rs1, int I_imm) {
        return T(((I_imm & 4095) << 20) | ((rd & 0x1f) << 7) | ((rs1 & 0x1f) << 15) | 19 | (0 << 12));
    }

    static T ANDI(unsigned rd, unsigned rs1, int I_imm) {
        return T(((I_imm & 4095) << 20) | ((rd & 0x1f) << 7) | ((rs1 & 0x1f) << 15) | 19 | (7 << 12));
    }

    static T SRLI(unsigned rd, unsigned rs1, unsigned shamt) {
        return T(((rd & 0x1f) << 7) | ((rs1 & 0x1f) << 15) | ((shamt & 31) << 20) | 19 | (5 << 12) | (0 << 25));
    }

    static T SRAI(unsigned rd, unsigned rs1, unsigned shamt) {
        return T(((rd & 0x1f) << 7) | ((rs1 & 0x1f) << 15) | ((shamt & 31) << 20) | 19 | (5 << 12) | (32 << 25));
    }

    static T SLLI(unsigned rd, unsigned rs1, unsigned shamt) {
        return T(((rd & 0x1f) << 7) | ((rs1 & 0x1f) << 15) | ((shamt & 31) << 20) | 19 | (1 << 12) | (0 << 25));
    }

    static T JAL(unsigned rd, int J_imm) {
        return T(111 | ((rd & 0x1f) << 7) | ((J_imm & (0b11111111 << 12)) | ((J_imm & (1 << 11)) << 9) | ((J_imm & 0b11111111110) << 20) | ((J_imm & (1 << 20)) << 11)));
    }

    static T JALR(unsigned rd, unsigned rs1, int I_imm) {
        return T(((I_imm & 4095) << 20) | ((rd & 0x1f) << 7) | ((rs1 & 0x1f) << 15) | 103 | (0 << 12));
    }

    static T BEQ(unsigned rs1, unsigned rs2, int B_imm) {
        return T(((((B_imm & 0b11110) << 7) | ((B_imm & (1 << 11)) >> 4)) | (((B_imm & (0b111111 << 5)) << 20) | ((B_imm & (1 << 12)) << 19))) | ((rs1 & 0x1f) << 15) | ((rs2 & 0x1f) << 20) | 99 | (0 << 12));
    }

    static T BNE(unsigned rs1, unsigned rs2, int B_imm) {
        return T(((((B_imm & 0b11110) << 7) | ((B_imm & (1 << 11)) >> 4)) | (((B_imm & (0b111111 << 5)) << 20) | ((B_imm & (1 << 12)) << 19))) | ((rs1 & 0x1f) << 15) | ((rs2 & 0x1f) << 20) | 99 | (1 << 12));
    }

    static T EBREAK() {
        return T(1048691);
    }
};



Compressed::Opcode decode_compressed(Instruction &instr) {
    using namespace Compressed;

    switch (instr.quadrant()) {
        case 0:
            switch (instr.c_opcode()) {
                case 0b000:
                    if (instr.c_format() == 0)
                        return C_Illegal;
                    else
                        return C_ADDI4SPN;

                case 0b010:
                    return C_LW;

                case 0b100:
                    return C_Reserved;

                case 0b110:
                    return C_SW;
            }
            break;

        case 1:
            switch (instr.c_opcode()) {
                case 0b000:
                    if (instr.c_format() == 1)
                        return C_NOP;
                    else
                        return C_ADDI;

                case 0b001:
                    return C_JAL;

                case 0b010:
                    return C_LI;

                case 0b011:
                    if (instr.c_rd() == 2)
                        return C_ADDI16SP;
                    else
                        return C_LUI;

                case 0b100:
                    switch (instr.c_f2_high()) {
                        case 0:
                            return C_SRLI;

                        case 1:
                            return C_SRAI;

                        case 2:
                            return C_ANDI;

                        case 3:
                            if (instr.c_b12()) {
                                switch (instr.c_f2_low()) {
                                    case 2:
                                    case 3:
                                        return C_Reserved;
                                }
                            } else {
                                switch (instr.c_f2_low()) {
                                    case 0:
                                        return C_SUB;
                                    case 1:
                                        return C_XOR;
                                    case 2:
                                        return C_OR;
                                    case 3:
                                        return C_AND;
                                }
                            }
                    }
                    break;

                case 0b101:
                    return C_J;

                case 0b110:
                    return C_BEQZ;

                case 0b111:
                    return C_BNEZ;
            }
            break;

        case 2:
            switch (instr.c_opcode()) {
                case 0b000:
                    return C_SLLI;

                case 0b010:
                    return C_LWSP;

                case 0b100:
                    if (instr.c_b12()) {
                        if (instr.c_rd()) {
                            if (instr.c_rs2()) {
                                return C_ADD;
                            } else {
                                return C_JALR;
                            }
                        } else {
                            return C_EBREAK;
                        }
                    } else {
                        if (instr.c_rs2()) {
                            return C_MV;
                        } else {
                            return C_JR;
                        }
                    }

                case 0b110:
                    return C_SWSP;
            }
            break;

        case 3:
            throw std::runtime_error("compressed instruction expected, but uncompressed found");
    }

    throw std::runtime_error("unknown compressed instruction");
}


Opcode::mapping expand_compressed(Instruction &instr, Compressed::Opcode op) {
    using namespace Opcode;
    using namespace Compressed;

    switch (op) {
        case C_Illegal:
            return UNDEF;

        case C_Reserved:
            throw std::runtime_error("Reserved compressed instruction detected");

        case C_NOP:
            instr = InstructionFactory::ADD(0, 0, 0);
            return ADD;

        case C_ADD:
            instr = InstructionFactory::ADD(instr.c_rd(), instr.c_rd(), instr.c_rs2());
            return ADD;

        case C_MV:
            instr = InstructionFactory::ADD(instr.c_rd(), 0, instr.c_rs2());
            return ADD;

        case C_AND:
            instr = InstructionFactory::AND(instr.c_rd_small(), instr.c_rd_small(), instr.c_rs2_small());
            return AND;

        case C_OR:
            instr = InstructionFactory::OR(instr.c_rd_small(), instr.c_rd_small(), instr.c_rs2_small());
            return OR;

        case C_XOR:
            instr = InstructionFactory::XOR(instr.c_rd_small(), instr.c_rd_small(), instr.c_rs2_small());
            return XOR;

        case C_SUB:
            instr = InstructionFactory::SUB(instr.c_rd_small(), instr.c_rd_small(), instr.c_rs2_small());
            return SUB;

        case C_LW:
            instr = InstructionFactory::LW(instr.c_rs2_small(), instr.c_rd_small(), C_LW_UIMM(instr.data()));
            return LW;

        case C_SW:
            instr = InstructionFactory::SW(instr.c_rd_small(), instr.c_rs2_small(), C_SW_UIMM(instr.data()));
            return SW;

        case C_ADDI4SPN: {
            unsigned n = C_ADDI4SPN_NZUIMM(instr.data());
            if (n == 0)
                return UNDEF;
            instr = InstructionFactory::ADDI(instr.c_rs2_small(), 2, n);
            return ADDI;
        }

        case C_ADDI:
            instr = InstructionFactory::ADDI(instr.c_rd(), instr.c_rd(), instr.c_imm());
            return ADDI;

        case C_JAL:
            instr = InstructionFactory::JAL(1, C_JAL_IMM(instr.data()));
            return JAL;

        case C_LI:
            instr = InstructionFactory::ADDI(instr.c_rd(), 0, instr.c_imm());
            return ADDI;

        case C_ADDI16SP: {
            auto n = C_ADDI16SP_NZIMM(instr.data());
            assert(n != 0);
            instr = InstructionFactory::ADDI(2, 2, n);
            return ADDI;
        }

        case C_LUI: {
            auto n = C_LUI_NZIMM(instr.data());
            assert (n != 0);
            assert (instr.c_rd() != 0);
            assert (instr.c_rd() != 2);
            instr = InstructionFactory::LUI(instr.c_rd(), n);
            return LUI;
        }

        case C_SRLI: {
            auto n = instr.c_uimm();
            if (n > 31)
                return UNDEF;
            instr = InstructionFactory::SRLI(instr.c_rd_small(), instr.c_rd_small(), n);
            return SRLI;
        }

        case C_SRAI: {
            auto n = instr.c_uimm();
            if (n > 31)
                return UNDEF;
            instr = InstructionFactory::SRAI(instr.c_rd_small(), instr.c_rd_small(), n);
            return SRAI;
        }

        case C_ANDI:
            instr = InstructionFactory::ANDI(instr.c_rd_small(), instr.c_rd_small(), instr.c_imm());
            return ANDI;

        case C_J:
            instr = InstructionFactory::JAL(0, C_J_IMM(instr.data()));
            return JAL;

        case C_BEQZ:
            instr = InstructionFactory::BEQ(instr.c_rd_small(), 0, C_BRANCH_IMM(instr.data()));
            return BEQ;

        case C_BNEZ:
            instr = InstructionFactory::BNE(instr.c_rd_small(), 0, C_BRANCH_IMM(instr.data()));
            return BNE;

        case C_SLLI: {
            auto n = instr.c_uimm();
            if (n > 31)
                return UNDEF;
            instr = InstructionFactory::SLLI(instr.c_rd_small(), instr.c_rd_small(), n);
            return SLLI;
        }

        case C_LWSP:
            instr = InstructionFactory::LW(instr.c_rd(), 2, C_LWSP_UIMM(instr.data()));
            return LW;

        case C_SWSP:
            instr = InstructionFactory::SW(2, instr.c_rs2(), C_SWSP_UIMM(instr.data()));
            return SW;

        case C_EBREAK:
            instr = InstructionFactory::EBREAK();
            return EBREAK;

        case C_JR:
            instr = InstructionFactory::JALR(0, instr.c_rd(), 0);
            return JALR;

        case C_JALR:
            instr = InstructionFactory::JALR(1, instr.c_rd(), 0);
            return JALR;
    }

    throw std::runtime_error("some compressed instruction not handled");
}


Opcode::mapping Instruction::decode_and_expand_compressed() {
    auto c_op = decode_compressed(*this);
    return expand_compressed(*this, c_op);
}


Opcode::mapping Instruction::decode_normal() {
    //NOTE: perhaps check constant fields inside the instructions to ensure that illegal instruction formats are detected (e.g. shamt extends into func7 field)
    using namespace Opcode;

    Instruction &instr = *this;

    switch (instr.opcode()) {
        case OP_LUI:
            return LUI;

        case OP_AUIPC:
            return AUIPC;

        case OP_JAL:
            return JAL;

        case OP_JALR: {
            assert (instr.funct3() == F3_JALR);
            return JALR;
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
                    assert (instr.funct7() == 0);
                    return SLLI;
                case F3_SRLI: {
                    switch (instr.funct7()) {
                        case F7_SRLI:
                            return SRLI;
                        case F7_SRAI:
                            return SRAI;
                    }
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
        }
    }

    return UNDEF;
}