#pragma once

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "util/common.h"
#include "core/common/clint.h"
#include "core/common/irq_if.h"
#include "core/common/bus_lock_if.h"
#include "csr.h"
#include "instr.h"
#include "syscall_if.h"
#include "trap.h"

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
// integrated in the ISS. Merge the *timedb* branch to use the timing_interface.
struct ISS;

struct timing_interface {
	virtual ~timing_interface() {}

	virtual void update_timing(Instruction instr, Opcode::Mapping op, ISS &iss) = 0;
};

struct instr_memory_interface {
	virtual ~instr_memory_interface() {}

	virtual uint32_t load_instr(uint64_t pc) = 0;
};

struct data_memory_interface {
	virtual ~data_memory_interface() {}

	virtual int32_t load_word(uint64_t addr) = 0;
	virtual int32_t load_half(uint64_t addr) = 0;
	virtual int32_t load_byte(uint64_t addr) = 0;
	virtual uint32_t load_uhalf(uint64_t addr) = 0;
	virtual uint32_t load_ubyte(uint64_t addr) = 0;

	virtual void store_word(uint64_t addr, uint32_t value) = 0;
	virtual void store_half(uint64_t addr, uint16_t value) = 0;
	virtual void store_byte(uint64_t addr, uint8_t value) = 0;

	virtual int32_t atomic_load_word(uint64_t addr) = 0;
    virtual void atomic_store_word(uint64_t addr, uint32_t value) = 0;
    virtual int32_t atomic_load_reserved_word(uint64_t addr) = 0;
    virtual bool atomic_store_conditional_word(uint64_t addr, uint32_t value) = 0;
    virtual void atomic_unlock() = 0;
};



enum class CoreExecStatus {
	Runnable,
	HitBreakpoint,
	Terminated,
};

struct ISS : public external_interrupt_target, public clint_interrupt_target, public iss_syscall_if {
	clint_if *clint = nullptr;
	instr_memory_interface *instr_mem = nullptr;
	data_memory_interface *mem = nullptr;
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

	void init(instr_memory_interface *instr_mem, data_memory_interface *data_mem, clint_if *clint,
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




class MemoryDMI {
    uint8_t *mem;
    uint64_t start;
    uint64_t size;
    uint64_t end;

    MemoryDMI(uint8_t *mem, uint64_t start, uint64_t size)
            : mem(mem), start(start), size(size), end(start+size) {
    }

public:
    static MemoryDMI create_start_end_mapping(uint8_t *mem, uint64_t start, uint64_t end) {
        assert (end > start);
        return create_start_size_mapping(mem, start, end-start);
    }

    static MemoryDMI create_start_size_mapping(uint8_t *mem, uint64_t start, uint64_t size) {
        assert (start + size > start);
        return MemoryDMI(mem, start, size);
    }

    uint8_t *get_mem_ptr() {
        return mem;
    }

    template <typename T>
    T *get_mem_ptr_to_global_addr(uint64_t addr) {
        assert (contains(addr));
        assert ((addr + sizeof(T)) <= end);
        return reinterpret_cast<T*>(mem + (addr - start));
    }

    uint64_t get_start() {
        return start;
    }

    uint64_t get_end() {
        return start+size;
    }

    uint64_t get_size() {
        return size;
    }

    bool contains(uint64_t addr) {
        return addr >= start && addr < end;
    }
};



/* For optimization, use DMI to fetch instructions. */
struct InstrMemoryProxy : public instr_memory_interface {
	MemoryDMI dmi;

	tlm_utils::tlm_quantumkeeper &quantum_keeper;
	sc_core::sc_time clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
	sc_core::sc_time access_delay = clock_cycle * 2;

	InstrMemoryProxy(const MemoryDMI &dmi, ISS &owner)
			: dmi(dmi), quantum_keeper(owner.quantum_keeper) {}

	virtual uint32_t load_instr(uint64_t pc) override {
		quantum_keeper.inc(access_delay);
		return *(dmi.get_mem_ptr_to_global_addr<uint32_t>(pc));
	}
};



struct CombinedMemoryInterface : public sc_core::sc_module,
								 public instr_memory_interface,
								 public data_memory_interface {
	ISS &iss;
	std::shared_ptr<bus_lock_if> bus_lock;
	uint64_t lr_addr = 0;

	tlm_utils::simple_initiator_socket<CombinedMemoryInterface> isock;
	tlm_utils::tlm_quantumkeeper &quantum_keeper;

    // optionally add DMI ranges for optimization
	sc_core::sc_time clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
	sc_core::sc_time dmi_access_delay = clock_cycle * 4;
	std::vector<MemoryDMI> dmi_ranges;

	CombinedMemoryInterface(sc_core::sc_module_name, ISS &owner)
			: iss(owner), quantum_keeper(iss.quantum_keeper) {
	}

	inline void _do_transaction(tlm::tlm_command cmd, uint64_t addr, uint8_t *data, unsigned num_bytes) {
		tlm::tlm_generic_payload trans;
		trans.set_command(cmd);
		trans.set_address(addr);
		trans.set_data_ptr(data);
		trans.set_data_length(num_bytes);

		sc_core::sc_time local_delay = quantum_keeper.get_local_time();

		isock->b_transport(trans, local_delay);

		assert(local_delay >= quantum_keeper.get_local_time());
		quantum_keeper.set(local_delay);
	}

	template <typename T>
	inline T _load_data(uint64_t addr) {
        // NOTE: a DMI load will not context switch (SystemC) and not modify the memory, hence should be able to postpone the lock after the dmi access
        bus_lock->wait_for_access_rights(iss.get_hart_id());

	    for (auto &e : dmi_ranges) {
	        if (e.contains(addr)) {
                quantum_keeper.inc(dmi_access_delay);

                T ans = *(e.get_mem_ptr_to_global_addr<T>(addr));
                return ans;
	        }
	    }

		T ans;
		_do_transaction(tlm::TLM_READ_COMMAND, addr, (uint8_t *)&ans, sizeof(T));
		return ans;
	}

	template <typename T>
	inline void _store_data(uint64_t addr, T value) {
        bus_lock->wait_for_access_rights(iss.get_hart_id());

        bool done = false;
        for (auto &e : dmi_ranges) {
            if (e.contains(addr)) {
                quantum_keeper.inc(dmi_access_delay);

                *(e.get_mem_ptr_to_global_addr<T>(addr)) = value;
                done = true;
            }
        }

        if (!done)
		    _do_transaction(tlm::TLM_WRITE_COMMAND, addr, (uint8_t *)&value, sizeof(T));
        atomic_unlock();
	}

	uint32_t load_instr(uint64_t addr) {
		return _load_data<uint32_t>(addr);
	}

	int32_t load_word(uint64_t addr) {
		return _load_data<int32_t>(addr);
	}
	int32_t load_half(uint64_t addr) {
		return _load_data<int16_t>(addr);
	}
	int32_t load_byte(uint64_t addr) {
		return _load_data<int8_t>(addr);
	}
	uint32_t load_uhalf(uint64_t addr) {
		return _load_data<uint16_t>(addr);
	}
	uint32_t load_ubyte(uint64_t addr) {
		return _load_data<uint8_t>(addr);
	}

	void store_word(uint64_t addr, uint32_t value) {
		_store_data(addr, value);
	}
	void store_half(uint64_t addr, uint16_t value) {
		_store_data(addr, value);
	}
	void store_byte(uint64_t addr, uint8_t value) {
		_store_data(addr, value);
	}

	virtual int32_t atomic_load_word(uint64_t addr) {
		bus_lock->lock(iss.get_hart_id());
		return load_word(addr);
	}
	virtual void atomic_store_word(uint64_t addr, uint32_t value) {
		assert (bus_lock->is_locked(iss.get_hart_id()));
		store_word(addr, value);
	}
	virtual int32_t atomic_load_reserved_word(uint64_t addr) {
		bus_lock->lock(iss.get_hart_id());
        lr_addr = addr;
		return load_word(addr);
	}
	virtual bool atomic_store_conditional_word(uint64_t addr, uint32_t value) {
		/* According to the RISC-V ISA, an implementation can fail each LR/SC sequence that does not satisfy the forward progress semantic.
		 * The lock is established by the LR instruction and the lock is kept while forward progress is maintained. */
		if (bus_lock->is_locked(iss.get_hart_id())) {
		    if (addr == lr_addr) {
                store_word(addr, value);
                return true;
            }
			atomic_unlock();
		}
		return false;
	}
	virtual void atomic_unlock() {
		bus_lock->unlock(iss.get_hart_id());
	}
};
