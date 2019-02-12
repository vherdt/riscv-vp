#ifndef RISCV_ISA_CLINT_H
#define RISCV_ISA_CLINT_H

#include <unordered_map>
#include "irq_if.h"

#include <tlm_utils/simple_target_socket.h>
#include <systemc>

struct clint_if {
	virtual ~clint_if() {}

	virtual uint64_t update_and_get_mtime() = 0;
};

struct CLINT : public clint_if, public sc_core::sc_module {
	//
	// core local interrupt controller (provides local timer interrupts with
	// memory mapped configuration)
	//
	// send out interrupt if: *mtime >= mtimecmp* and *mtimecmp != 0*
	//
	// *mtime* is a read-only 64 bit timer register shared by all CPUs (accessed
	// by MMIO and also mapped into the CSR address space of each CPU)
	//
	// for every CPU a *mtimecmp* register (read/write 64 bit) is available
	//
	//
	// Note: the software is responsible to ensure that no overflow occurs when
	// writing *mtimecmp* (see RISC-V privileged ISA spec.):
	//
	// # New comparand is in a1:a0.
	// li t0, -1
	// sw t0, mtimecmp    # No smaller than old value.
	// sw a1, mtimecmp+4  # No smaller than new value.
	// sw a0, mtimecmp    # New value.
	//

	static constexpr uint64_t scaler = 2000000;  // scale from PS resolution (default in SystemC) to US
	                                             // resolution (apparently required by FreeRTOS)

	tlm_utils::simple_target_socket<CLINT> tsock;

	sc_core::sc_time clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
	sc_core::sc_event irq_event;

	timer_interrupt_target *target_hart = nullptr;

	uint64_t mtime = 0;
	uint64_t mtimecmp = 0;  // zero means unused

	std::unordered_map<uint64_t, uint32_t *> addr_to_reg;

	SC_HAS_PROCESS(CLINT);

	CLINT(sc_core::sc_module_name) {
		tsock.register_b_transport(this, &CLINT::transport);

		addr_to_reg = {
		    {0xbff8, (uint32_t *)&mtime},
		    {0xbffc, (uint32_t *)&mtime + 1},
		    {0x4000, (uint32_t *)&mtimecmp},
		    {0x4004, (uint32_t *)&mtimecmp + 1},
		};

		SC_THREAD(run);
	}

	uint64_t update_and_get_mtime() override {
		auto now = sc_core::sc_time_stamp().value() / scaler;
		if (now > mtime)
			mtime = now;  // do not update backward in time (e.g. due to local
			              // quantums in tlm transaction processing)
		return mtime;
	}

	void run() {
		while (true) {
			sc_core::wait(irq_event);

			update_and_get_mtime();

			// std::cout << "[clint] process mtimecmp=" << mtimecmp << ",
			// mtime=" << mtime << std::endl;

			if (mtimecmp > 0 && mtime >= mtimecmp) {
				// std::cout << "[clint] set timer interrupt" << std::endl;
				target_hart->trigger_timer_interrupt(true);
			} else {
				// std::cout << "[clint] unset timer interrupt" << std::endl;
				target_hart->trigger_timer_interrupt(false);
				if (mtimecmp > 0) {
					auto time = sc_core::sc_time::from_value(mtime * scaler);
					auto goal = sc_core::sc_time::from_value(mtimecmp * scaler);
					// std::cout << "[clint] re-schedule event at trigger loop "
					// << goal - time << std::endl;
					irq_event.notify(goal - time);
				}
			}
		}
	}

	void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
		assert(trans.get_data_length() == 4);

		auto it = addr_to_reg.find(trans.get_address());
		assert(it != addr_to_reg.end());

		delay += clock_cycle;
		sc_core::sc_time now = sc_core::sc_time_stamp() + delay;

		if (trans.get_command() == tlm::TLM_READ_COMMAND) {
			mtime = now.value() / scaler;

			*((uint32_t *)trans.get_data_ptr()) = *it->second;
		} else if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
			assert(trans.get_address() != 0xbff8 && trans.get_address() != 0xbffc);  // mtime is readonly

			*it->second = *((uint32_t *)trans.get_data_ptr());

			// std::cout << "[clint] write mtimecmp=" << mtimecmp << ", mtime="
			// << mtime << std::endl;

			irq_event.notify(delay);
		} else {
			assert(false && "unsupported tlm command for CLINT access");
		}
	}
};

#endif  // RISCV_ISA_CLINT_H
