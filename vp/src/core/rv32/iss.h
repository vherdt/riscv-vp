#pragma once

#include "util/common.h"
#include "core/common/clint_if.h"
#include "core/common/irq_if.h"
#include "core/common/bus_lock_if.h"
#include "syscall_if.h"
#include "mem_if.h"
#include "csr.h"
#include "instr.h"
#include "trap.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/tlm_quantumkeeper.h>
#include <systemc>


struct RegFile {
	static constexpr unsigned NUM_REGS = 32;

	int32_t regs[NUM_REGS];

	RegFile();

	RegFile(const RegFile &other);

	void write(uint32_t index, int32_t value);

	int32_t read(uint32_t index);

	uint32_t shamt(uint32_t index);

	int32_t &operator[](const uint32_t idx);

	void show();

	enum e : uint16_t {
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


// NOTE: on this branch, currently the *simple-timing* model is still directly
// integrated in the ISS. Merge the *timedb* branch to use the timing_if.
struct ISS;

struct timing_if {
	virtual ~timing_if() {}

	virtual void update_timing(Instruction instr, Opcode::Mapping op, ISS &iss) = 0;
};


enum class CoreExecStatus {
	Runnable,
	HitBreakpoint,
	Terminated,
};

struct ISS : public external_interrupt_target, public clint_interrupt_target, public iss_syscall_if {
	clint_if *clint = nullptr;
	instr_memory_if *instr_mem = nullptr;
	data_memory_if *mem = nullptr;
	syscall_emulator_if *sys = nullptr;	// optional, if provided, the iss will intercept and handle syscalls directly
	RegFile regs;
	uint32_t pc = 0;
	uint32_t last_pc = 0;
	bool trace = false;
	bool shall_exit = false;
	csr_table csrs;
	uint64_t lr_sc_counter = 0;

	// last decoded and executed instruction and opcode
    Instruction instr;
    Opcode::Mapping op;

	CoreExecStatus status = CoreExecStatus::Runnable;
	std::unordered_set<uint32_t> breakpoints;
	bool debug_mode = false;

	sc_core::sc_event wfi_event;

	std::string systemc_name;
	tlm_utils::tlm_quantumkeeper quantum_keeper;
	sc_core::sc_time cycle_time;
	std::array<sc_core::sc_time, Opcode::NUMBER_OF_INSTRUCTIONS> instr_cycles;

	static constexpr int32_t REG_MIN = INT32_MIN;


	ISS(uint32_t hart_id);

	void exec_step();

	uint64_t _compute_and_get_current_cycles();

	void init(instr_memory_if *instr_mem, data_memory_if *data_mem, clint_if *clint,
	          uint32_t entrypoint, uint32_t sp);

	virtual void trigger_external_interrupt() override;

	virtual void clear_external_interrupt() override;

	virtual void trigger_timer_interrupt(bool status) override;

    virtual void trigger_software_interrupt(bool status) override;

	virtual void sys_exit() override;
	virtual uint32_t read_register(unsigned idx) override;
	virtual void write_register(unsigned idx, uint32_t value) override;

	uint32_t get_hart_id();


	void release_lr_sc_reservation() {
		lr_sc_counter = 0;
		mem->atomic_unlock();
	}


    uint32_t get_csr_value(uint32_t addr);
    void set_csr_value(uint32_t addr, uint32_t value);

    inline bool is_invalid_csr_access(uint32_t csr_addr, bool is_write) {
        bool csr_readonly = ((0xC00 & csr_addr) >> 10) == 3;
        return is_write && csr_readonly;
    }


    inline void trap_check_pc() {
        assert (!(pc & 0x1) && "not possible due to immediate formats and jump execution");

        if (unlikely((pc & 0x3) && (!csrs.misa.has_C_extension()))) {
            // NOTE: misaligned instruction address not possible on machines supporting compressed instructions
            raise_trap(EXC_INSTR_ADDR_MISALIGNED, pc);
        }
    }

    inline void trap_check_natural_alignment(uint32_t addr) {
        if (unlikely(addr & 0x3)) {
            raise_trap(EXC_INSTR_ADDR_MISALIGNED, addr);
        }
    }

    inline void execute_amo(Instruction &instr, std::function<int32_t(int32_t, int32_t)> operation) {
        uint32_t addr = regs[instr.rs1()];
        trap_check_natural_alignment(addr);
        uint32_t data;
        try {
            data = mem->atomic_load_word(addr);
        } catch (SimulationTrap &e) {
            if (e.reason == EXC_LOAD_ACCESS_FAULT)
                e.reason = EXC_STORE_AMO_ACCESS_FAULT;
            throw e;
        }
        uint32_t val = operation(data, regs[instr.rs2()]);
        mem->atomic_store_word(addr, val);
        regs[instr.rd()] = data;
    }

    inline void _trace_amo(const std::string &name, const Instruction &instr) {
    }


	void prepare_trap(SimulationTrap &e);

	void prepare_interrupt();

	void return_from_trap_handler();

	bool has_pending_enabled_interrupts();

	void switch_to_trap_handler();

	void performance_and_sync_update(Opcode::Mapping executed_op);

	void run_step();

	void run();

	void show();
};

/* Do not call the run function of the ISS directly but use one of the Runner
 * wrappers. */
struct DirectCoreRunner : public sc_core::sc_module {
	ISS &core;

	SC_HAS_PROCESS(DirectCoreRunner);

	DirectCoreRunner(ISS &core)
            : sc_module(sc_core::sc_module_name(core.systemc_name.c_str())), core(core) {
        SC_THREAD(run);
    }

	void run() {
        core.run();

        if (core.status == CoreExecStatus::HitBreakpoint) {
            throw std::runtime_error(
                    "Breakpoints are not supported in the direct runner, use the debug "
                    "runner instead.");
        }
        assert(core.status == CoreExecStatus::Terminated);

        sc_core::sc_stop();
	}
};