#include "iss.h"
#include "trap.h"

// to save *cout* format setting, see *ISS::show*
#include <boost/io/ios_state.hpp>
// for safe down-cast
#include <boost/lexical_cast.hpp>

const char *regnames[] = {
    "zero (x0)", "ra   (x1)", "sp   (x2)", "gp   (x3)", "tp   (x4)", "t0   (x5)", "t1   (x6)", "t2   (x7)",
    "s0/sp(x8)", "s1   (x9)", "a0  (x10)", "a1  (x11)", "a2  (x12)", "a3  (x13)", "a4  (x14)", "a5  (x15)",
    "a6  (x16)", "a7  (x17)", "s2  (x18)", "s3  (x19)", "s4  (x20)", "s5  (x21)", "s6  (x22)", "s7  (x23)",
    "s8  (x24)", "s9  (x25)", "s10 (x26)", "s11 (x27)", "t3  (x28)", "t4  (x29)", "t5  (x30)", "t6  (x31)",
};

int regcolors[] = {
#if defined(COLOR_THEME_DARK)
    0,  1,  2,  3,  4,  5,  6,  52, 8,  9,  53, 54, 55, 56, 57, 58,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
#elif defined(COLOR_THEME_LIGHT)
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 153, 154, 155, 156, 157, 158,
    116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
#else

#endif
};

RegFile::RegFile() {
	memset(regs, 0, sizeof(regs));
}

RegFile::RegFile(const RegFile &other) {
	memcpy(regs, other.regs, sizeof(regs));
}

void RegFile::write(uint32_t index, int32_t value) {
	assert(index <= x31);
	assert(index != x0);
	regs[index] = value;
}

int32_t RegFile::read(uint32_t index) {
	assert(index <= x31);
	return regs[index];
}

uint32_t RegFile::shamt(uint32_t index) {
	assert(index <= x31);
	return BIT_RANGE(regs[index], 4, 0);
}

int32_t &RegFile::operator[](const uint32_t idx) {
	return regs[idx];
}

#if defined(COLOR_THEME_LIGHT) || defined(COLOR_THEME_DARK)
#define COLORFRMT "\e[38;5;%um%s\e[39m"
#define COLORPRINT(fmt, data) fmt, data
#else
#define COLORFRMT "%s"
#define COLORPRINT(fmt, data) data
#endif

void RegFile::show() {
	for (unsigned i = 0; i < NUM_REGS; ++i) {
		printf(COLORFRMT " = %8x\n", COLORPRINT(regcolors[i], regnames[i]), regs[i]);
	}
}

ISS::ISS(uint32_t hart_id)
    : systemc_name("core-" + std::to_string(hart_id)) {
    csrs.mhartid.reg = hart_id;

	sc_core::sc_time qt = tlm::tlm_global_quantum::instance().get();
	cycle_time = sc_core::sc_time(10, sc_core::SC_NS);

	assert(qt >= cycle_time);
	assert(qt % cycle_time == sc_core::SC_ZERO_TIME);

	for (int i = 0; i < Opcode::NUMBER_OF_INSTRUCTIONS; ++i) instr_cycles[i] = cycle_time;

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
}

void ISS::exec_step() {
	assert (!(pc & 0x1) && (!(pc & 0x3) || csrs.misa.has_C_extension()) && "misaligned instruction");

	try {
        auto mem_word = instr_mem->load_instr(pc);
        instr = Instruction(mem_word);
    } catch (SimulationTrap &e) {
	    op = Opcode::UNDEF;
	    instr = Instruction(0);
	    throw;
	}

	if (instr.is_compressed()) {
		op = instr.decode_and_expand_compressed();
		pc += 2;
	} else {
		op = instr.decode_normal();
		pc += 4;
	}

	if (trace) {
		printf("core %2u: pc %8x: %s ", csrs.mhartid.reg, last_pc, Opcode::mappingStr[op]);
		switch (Opcode::getType(op)) {
			case Opcode::Type::R:
				printf(COLORFRMT ", " COLORFRMT ", " COLORFRMT, COLORPRINT(regcolors[instr.rd()], regnames[instr.rd()]),
				       COLORPRINT(regcolors[instr.rs1()], regnames[instr.rs1()]),
				       COLORPRINT(regcolors[instr.rs2()], regnames[instr.rs2()]));
				break;
			case Opcode::Type::I:
				printf(COLORFRMT ", " COLORFRMT ", 0x%x", COLORPRINT(regcolors[instr.rd()], regnames[instr.rd()]),
				       COLORPRINT(regcolors[instr.rs1()], regnames[instr.rs1()]), instr.I_imm());
				break;
			case Opcode::Type::S:
				printf(COLORFRMT ", " COLORFRMT ", 0x%x", COLORPRINT(regcolors[instr.rs1()], regnames[instr.rs1()]),
				       COLORPRINT(regcolors[instr.rs2()], regnames[instr.rs2()]), instr.S_imm());
				break;
			case Opcode::Type::B:
				printf(COLORFRMT ", " COLORFRMT ", 0x%x", COLORPRINT(regcolors[instr.rs1()], regnames[instr.rs1()]),
				       COLORPRINT(regcolors[instr.rs2()], regnames[instr.rs2()]), instr.B_imm());
				break;
			case Opcode::Type::U:
				printf(COLORFRMT ", 0x%x", COLORPRINT(regcolors[instr.rd()], regnames[instr.rd()]), instr.U_imm());
				break;
			case Opcode::Type::J:
				printf(COLORFRMT ", 0x%x", COLORPRINT(regcolors[instr.rd()], regnames[instr.rd()]), instr.J_imm());
				break;
			default:;
		}
		puts("");
	}

	switch (op) {
		case Opcode::UNDEF:
            std::cout << "WARNING: unknown instruction '" << std::to_string(instr.data()) << "' at address '" << std::to_string(last_pc) << "'" << std::endl;
		    raise_trap(EXC_ILLEGAL_INSTR, instr.data());

		case Opcode::ADDI:
			regs[instr.rd()] = regs[instr.rs1()] + instr.I_imm();
			break;

		case Opcode::SLTI:
			regs[instr.rd()] = regs[instr.rs1()] < instr.I_imm();
			break;

		case Opcode::SLTIU:
			regs[instr.rd()] = ((uint32_t)regs[instr.rs1()]) < ((uint32_t)instr.I_imm());
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

		case Opcode::JAL: {
            auto link = pc;
            pc = last_pc + instr.J_imm();
            trap_check_pc();
            regs[instr.rd()] = link;
        } break;

		case Opcode::JALR: {
            auto link = pc;
            pc = (regs[instr.rs1()] + instr.I_imm()) & ~1;
            trap_check_pc();
            regs[instr.rd()] = link;
		} break;

		case Opcode::SB: {
			uint32_t addr = regs[instr.rs1()] + instr.S_imm();
			mem->store_byte(addr, regs[instr.rs2()]);
		} break;

		case Opcode::SH: {
			uint32_t addr = regs[instr.rs1()] + instr.S_imm();
			mem->store_half(addr, regs[instr.rs2()]);
		} break;

		case Opcode::SW: {
			uint32_t addr = regs[instr.rs1()] + instr.S_imm();
			mem->store_word(addr, regs[instr.rs2()]);
		} break;

		case Opcode::LB: {
			uint32_t addr = regs[instr.rs1()] + instr.I_imm();
			regs[instr.rd()] = mem->load_byte(addr);
		} break;

		case Opcode::LH: {
			uint32_t addr = regs[instr.rs1()] + instr.I_imm();
			regs[instr.rd()] = mem->load_half(addr);
		} break;

		case Opcode::LW: {
			uint32_t addr = regs[instr.rs1()] + instr.I_imm();
			regs[instr.rd()] = mem->load_word(addr);
		} break;

		case Opcode::LBU: {
			uint32_t addr = regs[instr.rs1()] + instr.I_imm();
			regs[instr.rd()] = mem->load_ubyte(addr);
		} break;

		case Opcode::LHU: {
			uint32_t addr = regs[instr.rs1()] + instr.I_imm();
			regs[instr.rd()] = mem->load_uhalf(addr);
		} break;

		case Opcode::BEQ:
			if (regs[instr.rs1()] == regs[instr.rs2()]) {
                pc = last_pc + instr.B_imm();
                trap_check_pc();
            }
			break;

		case Opcode::BNE:
			if (regs[instr.rs1()] != regs[instr.rs2()]) {
                pc = last_pc + instr.B_imm();
                trap_check_pc();
            }
			break;

		case Opcode::BLT:
			if (regs[instr.rs1()] < regs[instr.rs2()]) {
                pc = last_pc + instr.B_imm();
                trap_check_pc();
            }
			break;

		case Opcode::BGE:
			if (regs[instr.rs1()] >= regs[instr.rs2()]) {
                pc = last_pc + instr.B_imm();
                trap_check_pc();
            }
			break;

		case Opcode::BLTU:
			if ((uint32_t)regs[instr.rs1()] < (uint32_t)regs[instr.rs2()]) {
                pc = last_pc + instr.B_imm();
                trap_check_pc();
            }
			break;

		case Opcode::BGEU:
			if ((uint32_t)regs[instr.rs1()] >= (uint32_t)regs[instr.rs2()]) {
                pc = last_pc + instr.B_imm();
                trap_check_pc();
            }
			break;

		case Opcode::FENCE:
		case Opcode::FENCE_I: {
			// not using out of order execution so can be ignored
		} break;

		case Opcode::ECALL: {
		    if (sys)
		        sys->execute_syscall(this);
		    else
		        raise_trap(EXC_ECALL_M_MODE, last_pc);
		} break;

		case Opcode::EBREAK: {
		    //TODO: also raise trap and let the SW deal with it?
            status = CoreExecStatus::HitBreakpoint;
        } break;

		case Opcode::CSRRW: {
			auto addr = instr.csr();
			if (is_invalid_csr_access(addr, true)) {
				raise_trap(EXC_ILLEGAL_INSTR, instr.data());
			} else {
				auto rd = instr.rd();
				auto rs1_val = regs[instr.rs1()];
				if (rd != RegFile::zero) {
					regs[instr.rd()] = get_csr_value(addr);
				}
				set_csr_value(addr, rs1_val);
			}
		} break;

		case Opcode::CSRRS: {
			auto addr = instr.csr();
			auto rs1 = instr.rs1();
			auto write = rs1 != RegFile::zero;
			if (is_invalid_csr_access(addr, write)) {
				raise_trap(EXC_ILLEGAL_INSTR, instr.data());
			} else {
				auto rd = instr.rd();
				auto rs1_val = regs[rs1];
				auto csr_val = get_csr_value(addr);
				if (rd != RegFile::zero)
					regs[rd] = csr_val;
				if (write)
					set_csr_value(addr, csr_val | rs1_val);
			}
		} break;

		case Opcode::CSRRC: {
			auto addr = instr.csr();
			auto rs1 = instr.rs1();
			auto write = rs1 != RegFile::zero;
			if (is_invalid_csr_access(addr, write)) {
				raise_trap(EXC_ILLEGAL_INSTR, instr.data());
			} else {
				auto rd = instr.rd();
				auto rs1_val = regs[rs1];
				auto csr_val = get_csr_value(addr);
				if (rd != RegFile::zero)
					regs[rd] = csr_val;
				if (write)
					set_csr_value(addr, csr_val & ~rs1_val);
			}
		} break;

		case Opcode::CSRRWI: {
			auto addr = instr.csr();
			if (is_invalid_csr_access(addr, true)) {
				raise_trap(EXC_ILLEGAL_INSTR, instr.data());
			} else {
				auto rd = instr.rd();
				if (rd != RegFile::zero) {
					regs[rd] = get_csr_value(addr);
				}
				set_csr_value(addr, instr.zimm());
			}
		} break;

		case Opcode::CSRRSI: {
			auto addr = instr.csr();
			auto zimm = instr.zimm();
			auto write = zimm != 0;
			if (is_invalid_csr_access(addr, true)) {
				raise_trap(EXC_ILLEGAL_INSTR, instr.data());
			} else {
				auto csr_val = get_csr_value(addr);
				auto rd = instr.rd();
				if (rd != RegFile::zero)
					regs[rd] = csr_val;
				if (write)
					set_csr_value(addr, csr_val | zimm);
			}
		} break;

		case Opcode::CSRRCI: {
			auto addr = instr.csr();
			auto zimm = instr.zimm();
			auto write = zimm != 0;
			if (is_invalid_csr_access(addr, true)) {
				raise_trap(EXC_ILLEGAL_INSTR, instr.data());
			} else {
				auto csr_val = get_csr_value(addr);
				auto rd = instr.rd();
				if (rd != RegFile::zero)
					regs[rd] = csr_val;
				if (write)
					set_csr_value(addr, csr_val & ~zimm);
			}
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
			// TODO: in multi-threaded system (or even if other components can
			// access the memory independently, e.g. through DMA) need to mark
			// this addr as reserved
			uint32_t addr = regs[instr.rs1()];
            trap_check_natural_alignment(addr);
            regs[instr.rd()] = mem->atomic_load_reserved_word(addr);
            lr_sc_counter = csrs.instret.reg + 17;  // this instruction + 16 additional ones to cover the forward progress property
		} break;

		case Opcode::SC_W: {
            uint32_t addr = regs[instr.rs1()];
            trap_check_natural_alignment(addr);
            uint32_t val  = regs[instr.rs2()];
            regs[instr.rd()] = 1;												        // failure by default (in case a trap is thrown)
            regs[instr.rd()] = mem->atomic_store_conditional_word(addr, val) ? 0 : 1;	// overwrite result (in case no trap is thrown)
			lr_sc_counter = 0;
		} break;

		// TODO: implement the aq and rl flags if necessary (check for all AMO
		// instructions)
		case Opcode::AMOSWAP_W: {
			_trace_amo("AMOSWAP_W", instr);
			execute_amo(instr, [](int32_t a, int32_t b) {
				return b;
			});
		} break;

		case Opcode::AMOADD_W: {
			_trace_amo("AMOADD_W", instr);
			execute_amo(instr, [](int32_t a, int32_t b) {
				return a + b;
			});
		} break;

		case Opcode::AMOXOR_W: {
			_trace_amo("AMOXOR_W", instr);
			execute_amo(instr, [](int32_t a, int32_t b) {
				return a ^ b;
			});
		} break;

		case Opcode::AMOAND_W: {
			_trace_amo("AMOAND_W", instr);
			execute_amo(instr, [](int32_t a, int32_t b) {
				return a & b;
			});
		} break;

		case Opcode::AMOOR_W: {
			_trace_amo("AMOOR_W", instr);
			execute_amo(instr, [](int32_t a, int32_t b) {
				return a | b;
			});
		} break;

		case Opcode::AMOMIN_W: {
			_trace_amo("AMOMIN_W", instr);
			execute_amo(instr, [](int32_t a, int32_t b) {
				return std::min(a, b);
			});
		} break;

		case Opcode::AMOMINU_W: {
			_trace_amo("AMOMINU_W", instr);
			execute_amo(instr, [](int32_t a, int32_t b) {
				return std::min((uint32_t)a, (uint32_t)b);
			});
		} break;

		case Opcode::AMOMAX_W: {
			_trace_amo("AMOMAX_W", instr);
			execute_amo(instr, [](int32_t a, int32_t b) {
				return std::max(a, b);
			});
		} break;

		case Opcode::AMOMAXU_W: {
			_trace_amo("AMOMAXU_W", instr);
			execute_amo(instr, [](int32_t a, int32_t b) {
				return std::max((uint32_t)a, (uint32_t)b);
			});
		} break;


		case Opcode::WFI:
			// NOTE: only a hint, can be implemented as NOP
			// std::cout << "[sim:wfi] CSR mstatus.mie " << csrs.mstatus->mie << std::endl;
			if (!has_pending_enabled_interrupts())
				sc_core::wait(wfi_event);
			break;

		case Opcode::SFENCE_VMA:
			// NOTE: not using MMU so far, so can be ignored
			break;

		case Opcode::URET:
		case Opcode::SRET:
			assert(false && "not implemented");
			break;
		case Opcode::MRET:
			return_from_trap_handler();
			break;

		default:
			throw std::runtime_error("unknown opcode");
	}
}

uint64_t ISS::_compute_and_get_current_cycles() {
	// Note: result is based on the default time resolution of SystemC (1 PS)
	sc_core::sc_time now = quantum_keeper.get_current_time();

	assert(now % cycle_time == sc_core::SC_ZERO_TIME);
	assert(now.value() % cycle_time.value() == 0);

	uint64_t num_cycles = now.value() / cycle_time.value();

	return num_cycles;
}


uint32_t ISS::get_csr_value(uint32_t addr) {
	switch (addr) {
		case CSR_TIME_ADDR:
		case CSR_MTIME_ADDR: {
			uint64_t mtime = clint->update_and_get_mtime();
			csrs.time.reg = mtime;
			return csrs.time.low;
		}

		case CSR_TIMEH_ADDR:
		case CSR_MTIMEH_ADDR: {
			uint64_t mtime = clint->update_and_get_mtime();
			csrs.time.reg = mtime;
			return csrs.time.high;
		}

		case CSR_MCYCLE_ADDR:
			csrs.cycle.reg = _compute_and_get_current_cycles();
			return csrs.cycle.low;

		case CSR_MCYCLEH_ADDR:
			csrs.cycle.reg = _compute_and_get_current_cycles();
			return csrs.cycle.high;

		case CSR_MINSTRET_ADDR:
			return csrs.instret.low;

		case CSR_MINSTRETH_ADDR:
			return csrs.instret.high;
	}

	if (!csrs.is_valid_csr32_addr(addr)) {
		raise_trap(EXC_ILLEGAL_INSTR, instr.data());
	}

	return csrs.default_read32(addr);
}

void ISS::set_csr_value(uint32_t addr, uint32_t value) {
	switch (addr) {
		case CSR_MTVEC_ADDR:
			csrs.mtvec.reg = value;
			if (csrs.mtvec.mode >= 1)
				csrs.mtvec.mode = 0;
			break;

			// read-only
		case CSR_MISA_ADDR:
		case CSR_MHARTID_ADDR:
			break;

	    default:
            csrs.default_write32(addr, value);
	}
}


void ISS::init(instr_memory_if *instr_mem, data_memory_if *data_mem, clint_if *clint,
               uint32_t entrypoint, uint32_t sp) {
	this->instr_mem = instr_mem;
	this->mem = data_mem;
	this->clint = clint;
	regs[RegFile::sp] = sp;
	pc = entrypoint;
}

void ISS::trigger_external_interrupt() {
	// std::cout << "[vp::iss] trigger external interrupt" << std::endl;
	csrs.mip.meip = true;
	wfi_event.notify(sc_core::SC_ZERO_TIME);
}

void ISS::clear_external_interrupt() {
	csrs.mip.meip = false;
}

void ISS::trigger_timer_interrupt(bool status) {
	csrs.mip.mtip = status;
	wfi_event.notify(sc_core::SC_ZERO_TIME);
}

void ISS::trigger_software_interrupt(bool status) {
	csrs.mip.msip = status;
	wfi_event.notify(sc_core::SC_ZERO_TIME);
}

void ISS::sys_exit() {
    shall_exit = true;
}

uint32_t ISS::read_register(unsigned idx) {
	return regs.read(idx);
}

void ISS::write_register(unsigned idx, uint32_t value) {
	regs.write(idx, value);
}

uint32_t ISS::get_hart_id() {
	return csrs.mhartid.reg;
}

void ISS::return_from_trap_handler() {
	//std::cout << "[vp::iss] return from trap handler @time " << quantum_keeper.get_current_time() << " to pc " << std::hex << csrs.mepc->reg << std::endl;

	// NOTE: assumes a SW based solution to store/re-store the execution
	// context, since this appears to be the RISC-V convention
	pc = csrs.mepc.reg;

	// NOTE: need to adapt when support for privilege levels beside M-mode is
	// added
	csrs.mstatus.mie = csrs.mstatus.mpie;
	csrs.mstatus.mpie = 1;
}

bool ISS::has_pending_enabled_interrupts() {
	assert(!csrs.mip.msip && "traps and syscalls are handled in the simulator");

	return csrs.mstatus.mie && ((csrs.mie.meie && csrs.mip.meip) || (csrs.mie.mtie && csrs.mip.mtip));
}


void ISS::prepare_trap(SimulationTrap &e) {
	pc = last_pc;	// undo any potential pc update
	csrs.mcause.interrupt = 0;
	csrs.mcause.exception_code = e.reason;
	csrs.mtval.reg = boost::lexical_cast<uint32_t>(e.mtval);
}

void ISS::prepare_interrupt() {
	assert(csrs.mstatus.mie);

	csrs.mcause.interrupt = 1;
	if (csrs.mie.meie && csrs.mip.meip) {
		csrs.mcause.exception_code = EXC_M_EXTERNAL_INTERRUPT;
	} else if (csrs.mie.mtie && csrs.mip.mtip) {
		csrs.mcause.exception_code = EXC_M_TIMER_INTERRUPT;
	} else {
		throw std::runtime_error("enabled pending interrupts must be available if this function is called");
	}
}


void ISS::switch_to_trap_handler() {
	//std::cout << "[vp::iss] switch to trap handler @time " << quantum_keeper.get_current_time() << " @last_pc " << std::hex << last_pc << " @pc " << pc << std::endl;

	// free any potential LR/SC bus lock before processing a trap/interrupt
	release_lr_sc_reservation();

	// for SW traps the address of the instruction causing the trap/interrupt
	// (i.e. last_pc, the address of the ECALL,EBREAK - better set pc=last_pc
	// before taking trap) for interrupts the address of the next instruction to
	// execute (since e.g. the RISC-V FreeRTOS port will not modify it)
	csrs.mepc.reg = pc;

	// deactivate interrupts before jumping to trap handler (SW can re-activate
	// if supported)
	csrs.mstatus.mpie = csrs.mstatus.mie;
	csrs.mstatus.mie = 0;

	// perform context switch to trap handler
	pc = csrs.mtvec.get_base_address();
}

void ISS::performance_and_sync_update(Opcode::Mapping executed_op) {
	++csrs.instret.reg;

	if (lr_sc_counter != 0 && lr_sc_counter < csrs.instret.reg)
		release_lr_sc_reservation();

	auto new_cycles = instr_cycles[executed_op];

	quantum_keeper.inc(new_cycles);
	if (quantum_keeper.need_sync()) {
		if (lr_sc_counter == 0)	// only an optimization, to avoid (SystemC) context switching while the bus is locked
			quantum_keeper.sync();
	}
}

void ISS::run_step() {
	assert(regs.read(0) == 0);

	last_pc = pc;
	try {
		exec_step();
	} catch (SimulationTrap &e) {
		prepare_trap(e);
		switch_to_trap_handler();
	}

	// NOTE: writes to zero register are supposedly allowed but must be ignored
	// (reset it after every instruction, instead of checking *rd != zero*
	// before every register write)
	regs.regs[regs.zero] = 0;

	if (has_pending_enabled_interrupts()) {
		prepare_interrupt();
		switch_to_trap_handler();
	}

	// Do not use a check *pc == last_pc* here. The reason is that due to
	// interrupts *pc* can be set to *last_pc* accidentally (when jumping back
	// to *mepc*).
	if (shall_exit)
		status = CoreExecStatus::Terminated;

	// speeds up the execution performance (non debug mode) significantly by
	// checking the additional flag first
	if (debug_mode && (breakpoints.find(pc) != breakpoints.end()))
		status = CoreExecStatus::HitBreakpoint;

	performance_and_sync_update(op);
}

void ISS::run() {
	// run a single step until either a breakpoint is hit or the execution
	// terminates
	do {
		run_step();
	} while (status == CoreExecStatus::Runnable);

	// force sync to make sure that no action is missed
	quantum_keeper.sync();
}

void ISS::show() {
	boost::io::ios_flags_saver ifs(std::cout);
	std::cout << "=[ core : " << csrs.mhartid.reg << " ]===========================" << std::endl;
	std::cout << "simulation time: " << sc_core::sc_time_stamp() << std::endl;
	regs.show();
	std::cout << "pc = " << std::hex << pc << std::endl;
	std::cout << "num-instr = " << std::dec << csrs.instret.reg << std::endl;
}
