#include "util/common.h"
#include "instr.h"
#include "trap.h"

#include <cassert>
#include <stdexcept>


constexpr uint32_t LUI_MASK            = 0b00000000000000000000000001111111;
constexpr uint32_t LUI_ENCODING        = 0b00000000000000000000000000110111;
constexpr uint32_t AUIPC_MASK          = 0b00000000000000000000000001111111;
constexpr uint32_t AUIPC_ENCODING      = 0b00000000000000000000000000010111;
constexpr uint32_t JAL_MASK            = 0b00000000000000000000000001111111;
constexpr uint32_t JAL_ENCODING        = 0b00000000000000000000000001101111;
constexpr uint32_t JALR_MASK           = 0b00000000000000000111000001111111;
constexpr uint32_t JALR_ENCODING       = 0b00000000000000000000000001100111;
constexpr uint32_t BEQ_MASK            = 0b00000000000000000111000001111111;
constexpr uint32_t BEQ_ENCODING        = 0b00000000000000000000000001100011;
constexpr uint32_t BNE_MASK            = 0b00000000000000000111000001111111;
constexpr uint32_t BNE_ENCODING        = 0b00000000000000000001000001100011;
constexpr uint32_t BLT_MASK            = 0b00000000000000000111000001111111;
constexpr uint32_t BLT_ENCODING        = 0b00000000000000000100000001100011;
constexpr uint32_t BGE_MASK            = 0b00000000000000000111000001111111;
constexpr uint32_t BGE_ENCODING        = 0b00000000000000000101000001100011;
constexpr uint32_t BLTU_MASK           = 0b00000000000000000111000001111111;
constexpr uint32_t BLTU_ENCODING       = 0b00000000000000000110000001100011;
constexpr uint32_t BGEU_MASK           = 0b00000000000000000111000001111111;
constexpr uint32_t BGEU_ENCODING       = 0b00000000000000000111000001100011;
constexpr uint32_t LB_MASK             = 0b00000000000000000111000001111111;
constexpr uint32_t LB_ENCODING         = 0b00000000000000000000000000000011;
constexpr uint32_t LH_MASK             = 0b00000000000000000111000001111111;
constexpr uint32_t LH_ENCODING         = 0b00000000000000000001000000000011;
constexpr uint32_t LW_MASK             = 0b00000000000000000111000001111111;
constexpr uint32_t LW_ENCODING         = 0b00000000000000000010000000000011;
constexpr uint32_t LBU_MASK            = 0b00000000000000000111000001111111;
constexpr uint32_t LBU_ENCODING        = 0b00000000000000000100000000000011;
constexpr uint32_t LHU_MASK            = 0b00000000000000000111000001111111;
constexpr uint32_t LHU_ENCODING        = 0b00000000000000000101000000000011;
constexpr uint32_t SB_MASK             = 0b00000000000000000111000001111111;
constexpr uint32_t SB_ENCODING         = 0b00000000000000000000000000100011;
constexpr uint32_t SH_MASK             = 0b00000000000000000111000001111111;
constexpr uint32_t SH_ENCODING         = 0b00000000000000000001000000100011;
constexpr uint32_t SW_MASK             = 0b00000000000000000111000001111111;
constexpr uint32_t SW_ENCODING         = 0b00000000000000000010000000100011;
constexpr uint32_t ADDI_MASK           = 0b00000000000000000111000001111111;
constexpr uint32_t ADDI_ENCODING       = 0b00000000000000000000000000010011;
constexpr uint32_t SLTI_MASK           = 0b00000000000000000111000001111111;
constexpr uint32_t SLTI_ENCODING       = 0b00000000000000000010000000010011;
constexpr uint32_t SLTIU_MASK          = 0b00000000000000000111000001111111;
constexpr uint32_t SLTIU_ENCODING      = 0b00000000000000000011000000010011;
constexpr uint32_t XORI_MASK           = 0b00000000000000000111000001111111;
constexpr uint32_t XORI_ENCODING       = 0b00000000000000000100000000010011;
constexpr uint32_t ORI_MASK            = 0b00000000000000000111000001111111;
constexpr uint32_t ORI_ENCODING        = 0b00000000000000000110000000010011;
constexpr uint32_t ANDI_MASK           = 0b00000000000000000111000001111111;
constexpr uint32_t ANDI_ENCODING       = 0b00000000000000000111000000010011;
constexpr uint32_t SLLI_MASK           = 0b11111110000000000111000001111111;
constexpr uint32_t SLLI_ENCODING       = 0b00000000000000000001000000010011;
constexpr uint32_t SRLI_MASK           = 0b11111110000000000111000001111111;
constexpr uint32_t SRLI_ENCODING       = 0b00000000000000000101000000010011;
constexpr uint32_t SRAI_MASK           = 0b11111110000000000111000001111111;
constexpr uint32_t SRAI_ENCODING       = 0b01000000000000000101000000010011;
constexpr uint32_t ADD_MASK            = 0b11111110000000000111000001111111;
constexpr uint32_t ADD_ENCODING        = 0b00000000000000000000000000110011;
constexpr uint32_t SUB_MASK            = 0b11111110000000000111000001111111;
constexpr uint32_t SUB_ENCODING        = 0b01000000000000000000000000110011;
constexpr uint32_t SLL_MASK            = 0b11111110000000000111000001111111;
constexpr uint32_t SLL_ENCODING        = 0b00000000000000000001000000110011;
constexpr uint32_t SLT_MASK            = 0b11111110000000000111000001111111;
constexpr uint32_t SLT_ENCODING        = 0b00000000000000000010000000110011;
constexpr uint32_t SLTU_MASK           = 0b11111110000000000111000001111111;
constexpr uint32_t SLTU_ENCODING       = 0b00000000000000000011000000110011;
constexpr uint32_t XOR_MASK            = 0b11111110000000000111000001111111;
constexpr uint32_t XOR_ENCODING        = 0b00000000000000000100000000110011;
constexpr uint32_t SRL_MASK            = 0b11111110000000000111000001111111;
constexpr uint32_t SRL_ENCODING        = 0b00000000000000000101000000110011;
constexpr uint32_t SRA_MASK            = 0b11111110000000000111000001111111;
constexpr uint32_t SRA_ENCODING        = 0b01000000000000000101000000110011;
constexpr uint32_t OR_MASK             = 0b11111110000000000111000001111111;
constexpr uint32_t OR_ENCODING         = 0b00000000000000000110000000110011;
constexpr uint32_t AND_MASK            = 0b11111110000000000111000001111111;
constexpr uint32_t AND_ENCODING        = 0b00000000000000000111000000110011;
constexpr uint32_t FENCE_MASK          = 0b00000000000000000111000001111111;
constexpr uint32_t FENCE_ENCODING      = 0b00000000000000000000000000001111;
constexpr uint32_t FENCE_I_MASK        = 0b00000000000000000111000001111111;
constexpr uint32_t FENCE_I_ENCODING    = 0b00000000000000000001000000001111;
constexpr uint32_t ECALL_MASK          = 0b11111111111100000111000001111111;
constexpr uint32_t ECALL_ENCODING      = 0b00000000000000000000000001110011;
constexpr uint32_t EBREAK_MASK         = 0b11111111111100000111000001111111;
constexpr uint32_t EBREAK_ENCODING     = 0b00000000000100000000000001110011;
constexpr uint32_t CSRRW_MASK          = 0b00000000000000000111000001111111;
constexpr uint32_t CSRRW_ENCODING      = 0b00000000000000000001000001110011;
constexpr uint32_t CSRRS_MASK          = 0b00000000000000000111000001111111;
constexpr uint32_t CSRRS_ENCODING      = 0b00000000000000000010000001110011;
constexpr uint32_t CSRRC_MASK          = 0b00000000000000000111000001111111;
constexpr uint32_t CSRRC_ENCODING      = 0b00000000000000000011000001110011;
constexpr uint32_t CSRRWI_MASK         = 0b00000000000000000111000001111111;
constexpr uint32_t CSRRWI_ENCODING     = 0b00000000000000000101000001110011;
constexpr uint32_t CSRRSI_MASK         = 0b00000000000000000111000001111111;
constexpr uint32_t CSRRSI_ENCODING     = 0b00000000000000000110000001110011;
constexpr uint32_t CSRRCI_MASK         = 0b00000000000000000111000001111111;
constexpr uint32_t CSRRCI_ENCODING     = 0b00000000000000000111000001110011;
constexpr uint32_t MUL_MASK            = 0b11111110000000000111000001111111;
constexpr uint32_t MUL_ENCODING        = 0b00000010000000000000000000110011;
constexpr uint32_t MULH_MASK           = 0b11111110000000000111000001111111;
constexpr uint32_t MULH_ENCODING       = 0b00000010000000000001000000110011;
constexpr uint32_t MULHSU_MASK         = 0b11111110000000000111000001111111;
constexpr uint32_t MULHSU_ENCODING     = 0b00000010000000000010000000110011;
constexpr uint32_t MULHU_MASK          = 0b11111110000000000111000001111111;
constexpr uint32_t MULHU_ENCODING      = 0b00000010000000000011000000110011;
constexpr uint32_t DIV_MASK            = 0b11111110000000000111000001111111;
constexpr uint32_t DIV_ENCODING        = 0b00000010000000000100000000110011;
constexpr uint32_t DIVU_MASK           = 0b11111110000000000111000001111111;
constexpr uint32_t DIVU_ENCODING       = 0b00000010000000000101000000110011;
constexpr uint32_t REM_MASK            = 0b11111110000000000111000001111111;
constexpr uint32_t REM_ENCODING        = 0b00000010000000000110000000110011;
constexpr uint32_t REMU_MASK           = 0b11111110000000000111000001111111;
constexpr uint32_t REMU_ENCODING       = 0b00000010000000000111000000110011;
constexpr uint32_t LR_W_MASK           = 0b11111001111100000111000001111111;
constexpr uint32_t LR_W_ENCODING       = 0b00010000000000000010000000101111;
constexpr uint32_t SC_W_MASK           = 0b11111000000000000111000001111111;
constexpr uint32_t SC_W_ENCODING       = 0b00011000000000000010000000101111;
constexpr uint32_t AMOSWAP_W_MASK      = 0b11111000000000000111000001111111;
constexpr uint32_t AMOSWAP_W_ENCODING  = 0b00001000000000000010000000101111;
constexpr uint32_t AMOADD_W_MASK       = 0b11111000000000000111000001111111;
constexpr uint32_t AMOADD_W_ENCODING   = 0b00000000000000000010000000101111;
constexpr uint32_t AMOXOR_W_MASK       = 0b11111000000000000111000001111111;
constexpr uint32_t AMOXOR_W_ENCODING   = 0b00100000000000000010000000101111;
constexpr uint32_t AMOAND_W_MASK       = 0b11111000000000000111000001111111;
constexpr uint32_t AMOAND_W_ENCODING   = 0b01100000000000000010000000101111;
constexpr uint32_t AMOOR_W_MASK        = 0b11111000000000000111000001111111;
constexpr uint32_t AMOOR_W_ENCODING    = 0b01000000000000000010000000101111;
constexpr uint32_t AMOMIN_W_MASK       = 0b11111000000000000111000001111111;
constexpr uint32_t AMOMIN_W_ENCODING   = 0b10000000000000000010000000101111;
constexpr uint32_t AMOMAX_W_MASK       = 0b11111000000000000111000001111111;
constexpr uint32_t AMOMAX_W_ENCODING   = 0b10100000000000000010000000101111;
constexpr uint32_t AMOMINU_W_MASK      = 0b11111000000000000111000001111111;
constexpr uint32_t AMOMINU_W_ENCODING  = 0b11000000000000000010000000101111;
constexpr uint32_t AMOMAXU_W_MASK      = 0b11111000000000000111000001111111;
constexpr uint32_t AMOMAXU_W_ENCODING  = 0b11100000000000000010000000101111;
constexpr uint32_t URET_MASK           = 0b11111111111111111111111111111111;
constexpr uint32_t URET_ENCODING       = 0b00000000001000000000000001110011;
constexpr uint32_t SRET_MASK           = 0b11111111111111111111111111111111;
constexpr uint32_t SRET_ENCODING       = 0b00010000001000000000000001110011;
constexpr uint32_t MRET_MASK           = 0b11111111111111111111111111111111;
constexpr uint32_t MRET_ENCODING       = 0b00110000001000000000000001110011;
constexpr uint32_t WFI_MASK            = 0b11111111111111111111111111111111;
constexpr uint32_t WFI_ENCODING        = 0b00010000010100000000000001110011;
constexpr uint32_t SFENCE_VMA_MASK     = 0b11111110000000000111111111111111;
constexpr uint32_t SFENCE_VMA_ENCODING = 0b00010010000000000000000001110011;


#define MATCH_AND_RETURN_INSTR(instr)									\
	if (unlikely((data() & (instr ## _MASK)) != (instr ## _ENCODING)))	\
		return UNDEF;                        							\
	return instr;


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

std::array<const char*, Opcode::NUMBER_OF_INSTRUCTIONS> Opcode::mappingStr = {
    "ZERO-INVALID",

    // RV32I base instruction set
    "LUI",
    "AUIPC",
    "JAL",
    "JALR",
    "BEQ",
    "BNE",
    "BLT",
    "BGE",
    "BLTU",
    "BGEU",
    "LB",
    "LH",
    "LW",
    "LBU",
    "LHU",
    "SB",
    "SH",
    "SW",
    "ADDI",
    "SLTI",
    "SLTIU",
    "XORI",
    "ORI",
    "ANDI",
    "SLLI",
    "SRLI",
    "SRAI",
    "ADD",
    "SUB",
    "SLL",
    "SLT",
    "SLTU",
    "XOR",
    "SRL",
    "SRA",
    "OR",
    "AND",
    "FENCE",
    "ECALL",
    "EBREAK",

    // Zifencei standard extension
	"FENCE_I",

	// Zicsr standard extension
    "CSRRW",
    "CSRRS",
    "CSRRC",
    "CSRRWI",
    "CSRRSI",
    "CSRRCI",

    // RV32M Standard Extension
    "MUL",
    "MULH",
    "MULHSU",
    "MULHU",
    "DIV",
    "DIVU",
    "REM",
    "REMU",

    // RV32A Standard Extension
    "LR_W",
    "SC_W",
    "AMOSWAP_W",
    "AMOADD_W",
    "AMOXOR_W",
    "AMOAND_W",
    "AMOOR_W",
    "AMOMIN_W",
    "AMOMAX_W",
    "AMOMINU_W",
    "AMOMAXU_W",

    // privileged instructions
    "URET",
    "SRET",
    "MRET",
    "WFI",
    "SFENCE_VMA",
};

Opcode::Type Opcode::getType(Opcode::Mapping mapping) {
	switch (mapping) {
		case SLLI:
		case SRLI:
		case SRAI:
		case ADD:
		case SUB:
		case SLL:
		case SLT:
		case SLTU:
		case XOR:
		case SRL:
		case SRA:
		case OR:
		case AND:
		case MUL:
		case MULH:
		case MULHSU:
		case MULHU:
		case DIV:
		case DIVU:
		case REM:
		case REMU:
			return Type::R;
		case JALR:
		case LB:
		case LH:
		case LW:
		case LBU:
		case LHU:
		case ADDI:
		case SLTI:
		case SLTIU:
		case XORI:
		case ORI:
		case ANDI:
			return Type::I;
		case SB:
		case SH:
		case SW:
			return Type::S;
		case BEQ:
		case BNE:
		case BLT:
		case BGE:
		case BLTU:
		case BGEU:
			return Type::B;
		case LUI:
		case AUIPC:
			return Type::U;
		case JAL:
			return Type::J;

		default:
			return Type::UNKNOWN;
	}
}

unsigned C_ADDI4SPN_NZUIMM(uint32_t n) {
	return (BIT_SLICE(n, 12, 11) << 4) | (BIT_SLICE(n, 10, 7) << 6) | (BIT_SINGLE_P1(n, 6) << 2) |
	       (BIT_SINGLE_P1(n, 5) << 3);
}

unsigned C_LW_UIMM(uint32_t n) {
	return (BIT_SLICE(n, 12, 10) << 3) | (BIT_SINGLE_P1(n, 6) << 2) | (BIT_SINGLE_P1(n, 5) << 6);
}

unsigned C_SW_UIMM(uint32_t n) {
	return C_LW_UIMM(n);
}

int32_t C_JAL_IMM(int32_t n) {
	return EXTRACT_SIGN_BIT(n, 12, 11) | BIT_SINGLE_PN(n, 11, 4) | (BIT_SLICE(n, 10, 9) << 8) |
	       BIT_SINGLE_PN(n, 8, 10) | BIT_SINGLE_PN(n, 7, 6) | BIT_SINGLE_PN(n, 6, 7) | (BIT_SLICE(n, 5, 3) << 1) |
	       BIT_SINGLE_PN(n, 2, 5);
}

int32_t C_ADDI16SP_NZIMM(int32_t n) {
	return EXTRACT_SIGN_BIT(n, 12, 9) | BIT_SINGLE_PN(n, 6, 4) | BIT_SINGLE_PN(n, 5, 6) | (BIT_SLICE(n, 4, 3) << 7) |
	       BIT_SINGLE_PN(n, 2, 5);
}

int32_t C_LUI_NZIMM(int32_t n) {
	return EXTRACT_SIGN_BIT(n, 12, 17) | (BIT_SLICE(n, 6, 2) << 12);
}

int32_t C_J_IMM(int32_t n) {
	return C_JAL_IMM(n);
}

int32_t C_BRANCH_IMM(int32_t n) {
	return EXTRACT_SIGN_BIT(n, 12, 8) | (BIT_SLICE(n, 11, 10) << 3) | (BIT_SLICE(n, 6, 5) << 6) |
	       (BIT_SLICE(n, 4, 3) << 1) | BIT_SINGLE_PN(n, 2, 5);
}

uint32_t C_LWSP_UIMM(uint32_t n) {
	return BIT_SINGLE_PN(n, 12, 5) | (BIT_SLICE(n, 6, 4) << 2) | (BIT_SLICE(n, 3, 2) << 6);
}

uint32_t C_SWSP_UIMM(uint32_t n) {
	return (BIT_SLICE(n, 12, 9) << 2) | (BIT_SLICE(n, 8, 7) << 6);
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
		return T((((S_imm & 0b11111) << 7) | ((S_imm & (0b1111111 << 5)) << 20)) | ((rs1 & 0x1f) << 15) |
		         ((rs2 & 0x1f) << 20) | 35 | (2 << 12));
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
		return T(111 | ((rd & 0x1f) << 7) |
		         ((J_imm & (0b11111111 << 12)) | ((J_imm & (1 << 11)) << 9) | ((J_imm & 0b11111111110) << 20) |
		          ((J_imm & (1 << 20)) << 11)));
	}

	static T JALR(unsigned rd, unsigned rs1, int I_imm) {
		return T(((I_imm & 4095) << 20) | ((rd & 0x1f) << 7) | ((rs1 & 0x1f) << 15) | 103 | (0 << 12));
	}

	static T BEQ(unsigned rs1, unsigned rs2, int B_imm) {
		return T(((((B_imm & 0b11110) << 7) | ((B_imm & (1 << 11)) >> 4)) |
		          (((B_imm & (0b111111 << 5)) << 20) | ((B_imm & (1 << 12)) << 19))) |
		         ((rs1 & 0x1f) << 15) | ((rs2 & 0x1f) << 20) | 99 | (0 << 12));
	}

	static T BNE(unsigned rs1, unsigned rs2, int B_imm) {
		return T(((((B_imm & 0b11110) << 7) | ((B_imm & (1 << 11)) >> 4)) |
		          (((B_imm & (0b111111 << 5)) << 20) | ((B_imm & (1 << 12)) << 19))) |
		         ((rs1 & 0x1f) << 15) | ((rs2 & 0x1f) << 20) | 99 | (1 << 12));
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

Opcode::Mapping expand_compressed(Instruction &instr, Compressed::Opcode op) {
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
			assert(n != 0);
			assert(instr.c_rd() != 0);
			assert(instr.c_rd() != 2);
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

Opcode::Mapping Instruction::decode_and_expand_compressed() {
	auto c_op = decode_compressed(*this);
	return expand_compressed(*this, c_op);
}

Opcode::Mapping Instruction::decode_normal() {
	using namespace Opcode;

	Instruction &instr = *this;

	switch (instr.opcode()) {
		case OP_LUI:
			MATCH_AND_RETURN_INSTR(LUI);

		case OP_AUIPC:
            MATCH_AND_RETURN_INSTR(AUIPC);

		case OP_JAL:
            MATCH_AND_RETURN_INSTR(JAL);

		case OP_JALR: {
            MATCH_AND_RETURN_INSTR(JALR);
		}

		case OP_BEQ: {
			switch (instr.funct3()) {
				case F3_BEQ:
                    MATCH_AND_RETURN_INSTR(BEQ);
				case F3_BNE:
                    MATCH_AND_RETURN_INSTR(BNE);
				case F3_BLT:
                    MATCH_AND_RETURN_INSTR(BLT);
				case F3_BGE:
                    MATCH_AND_RETURN_INSTR(BGE);
				case F3_BLTU:
                    MATCH_AND_RETURN_INSTR(BLTU);
				case F3_BGEU:
                    MATCH_AND_RETURN_INSTR(BGEU);
			}
			break;
		}

		case OP_LB: {
			switch (instr.funct3()) {
				case F3_LB:
                    MATCH_AND_RETURN_INSTR(LB);
				case F3_LH:
                    MATCH_AND_RETURN_INSTR(LH);
				case F3_LW:
                    MATCH_AND_RETURN_INSTR(LW);
				case F3_LBU:
                    MATCH_AND_RETURN_INSTR(LBU);
				case F3_LHU:
                    MATCH_AND_RETURN_INSTR(LHU);
			}
			break;
		}

		case OP_SB: {
			switch (instr.funct3()) {
				case F3_SB:
                    MATCH_AND_RETURN_INSTR(SB);
				case F3_SH:
                    MATCH_AND_RETURN_INSTR(SH);
				case F3_SW:
                    MATCH_AND_RETURN_INSTR(SW);
			}
			break;
		}

		case OP_ADDI: {
			switch (instr.funct3()) {
				case F3_ADDI:
                    MATCH_AND_RETURN_INSTR(ADDI);
				case F3_SLTI:
                    MATCH_AND_RETURN_INSTR(SLTI);
				case F3_SLTIU:
                    MATCH_AND_RETURN_INSTR(SLTIU);
				case F3_XORI:
                    MATCH_AND_RETURN_INSTR(XORI);
				case F3_ORI:
                    MATCH_AND_RETURN_INSTR(ORI);
				case F3_ANDI:
                    MATCH_AND_RETURN_INSTR(ANDI);
				case F3_SLLI:
                    MATCH_AND_RETURN_INSTR(SLLI);
				case F3_SRLI: {
					switch (instr.funct7()) {
						case F7_SRLI:
							MATCH_AND_RETURN_INSTR(SRLI);
						case F7_SRAI:
                            MATCH_AND_RETURN_INSTR(SRAI);
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
                            MATCH_AND_RETURN_INSTR(ADD);
						case F3_SLL:
                            MATCH_AND_RETURN_INSTR(SLL);
						case F3_SLT:
                            MATCH_AND_RETURN_INSTR(SLT);
						case F3_SLTU:
                            MATCH_AND_RETURN_INSTR(SLTU);
						case F3_XOR:
                            MATCH_AND_RETURN_INSTR(XOR);
						case F3_SRL:
                            MATCH_AND_RETURN_INSTR(SRL);
						case F3_OR:
                            MATCH_AND_RETURN_INSTR(OR);
						case F3_AND:
                            MATCH_AND_RETURN_INSTR(AND);
					}
					break;

				case F7_SUB:
					switch (instr.funct3()) {
						case F3_SUB:
                            MATCH_AND_RETURN_INSTR(SUB);
						case F3_SRA:
                            MATCH_AND_RETURN_INSTR(SRA);
					}
					break;

				case F7_MUL:
					switch (instr.funct3()) {
						case F3_MUL:
                            MATCH_AND_RETURN_INSTR(MUL);
						case F3_MULH:
                            MATCH_AND_RETURN_INSTR(MULH);
						case F3_MULHSU:
                            MATCH_AND_RETURN_INSTR(MULHSU);
						case F3_MULHU:
                            MATCH_AND_RETURN_INSTR(MULHU);
						case F3_DIV:
                            MATCH_AND_RETURN_INSTR(DIV);
						case F3_DIVU:
                            MATCH_AND_RETURN_INSTR(DIVU);
						case F3_REM:
                            MATCH_AND_RETURN_INSTR(REM);
						case F3_REMU:
                            MATCH_AND_RETURN_INSTR(REMU);
					}
					break;
			}
			break;
		}

		case OP_FENCE: {
			switch (instr.funct3()) {
				case F3_FENCE:
                    MATCH_AND_RETURN_INSTR(FENCE);
				case F3_FENCE_I:
                    MATCH_AND_RETURN_INSTR(FENCE_I);
			}
			break;
		}

		case OP_ECALL: {
			switch (instr.funct3()) {
				case F3_SYS: {
					switch (instr.funct12()) {
						case F12_ECALL:
                            MATCH_AND_RETURN_INSTR(ECALL);
						case F12_EBREAK:
                            MATCH_AND_RETURN_INSTR(EBREAK);
						case F12_URET:
                            MATCH_AND_RETURN_INSTR(URET);
						case F12_SRET:
                            MATCH_AND_RETURN_INSTR(SRET);
						case F12_MRET:
                            MATCH_AND_RETURN_INSTR(MRET);
						case F12_WFI:
                            MATCH_AND_RETURN_INSTR(WFI);
						default:
                            MATCH_AND_RETURN_INSTR(SFENCE_VMA);
					}
					break;
				}
				case F3_CSRRW:
                    MATCH_AND_RETURN_INSTR(CSRRW);
				case F3_CSRRS:
                    MATCH_AND_RETURN_INSTR(CSRRS);
				case F3_CSRRC:
                    MATCH_AND_RETURN_INSTR(CSRRC);
				case F3_CSRRWI:
                    MATCH_AND_RETURN_INSTR(CSRRWI);
				case F3_CSRRSI:
                    MATCH_AND_RETURN_INSTR(CSRRSI);
				case F3_CSRRCI:
                    MATCH_AND_RETURN_INSTR(CSRRCI);
			}
			break;
		}

		case OP_AMO: {
			switch (instr.funct5()) {
				case F5_LR_W:
                    MATCH_AND_RETURN_INSTR(LR_W);
				case F5_SC_W:
                    MATCH_AND_RETURN_INSTR(SC_W);
				case F5_AMOSWAP_W:
                    MATCH_AND_RETURN_INSTR(AMOSWAP_W);
				case F5_AMOADD_W:
                    MATCH_AND_RETURN_INSTR(AMOADD_W);
				case F5_AMOXOR_W:
                    MATCH_AND_RETURN_INSTR(AMOXOR_W);
				case F5_AMOAND_W:
                    MATCH_AND_RETURN_INSTR(AMOAND_W);
				case F5_AMOOR_W:
                    MATCH_AND_RETURN_INSTR(AMOOR_W);
				case F5_AMOMIN_W:
                    MATCH_AND_RETURN_INSTR(AMOMIN_W);
				case F5_AMOMAX_W:
                    MATCH_AND_RETURN_INSTR(AMOMAX_W);
				case F5_AMOMINU_W:
                    MATCH_AND_RETURN_INSTR(AMOMINU_W);
				case F5_AMOMAXU_W:
                    MATCH_AND_RETURN_INSTR(AMOMAXU_W);
			}
		}
	}

	return UNDEF;
}
