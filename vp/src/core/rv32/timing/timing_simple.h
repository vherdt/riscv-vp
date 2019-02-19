#pragma once

#include "../iss.h"

struct SimpleTimingDecorator : public timing_if {
	std::array<sc_core::sc_time, Opcode::NUMBER_OF_INSTRUCTIONS> instr_cycles;
	sc_core::sc_time cycle_time = sc_core::sc_time(10, sc_core::SC_NS);

	SimpleTimingDecorator() {
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

	void on_begin_exec_step(Instruction instr, Opcode::mapping op, ISS &iss) override {
		auto new_cycles = instr_cycles[op];

		iss.quantum_keeper.inc(new_cycles);
	}
};
