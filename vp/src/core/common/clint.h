#ifndef RISCV_ISA_CLINT_H
#define RISCV_ISA_CLINT_H

#include "irq_if.h"
#include "clint_if.h"

#include <tlm_utils/simple_target_socket.h>
#include <systemc>

#include <unordered_map>


template <unsigned NumberOfCores>
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

    static_assert(NumberOfCores < 4096, "out of bound"); // stay within the allocated address range

	static constexpr uint64_t scaler = 1000000;  // scale from PS resolution (default in SystemC) to US
	                                             // resolution (apparently required by FreeRTOS)

	tlm_utils::simple_target_socket<CLINT> tsock;

	sc_core::sc_time clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
	sc_core::sc_event irq_event;

	uint64_t mtime = 0;
	std::array<uint64_t, NumberOfCores> mtimecmp{};
    std::array<uint32_t, NumberOfCores> msip{};

	std::array<clint_interrupt_target*, NumberOfCores> target_harts{};

	std::unordered_map<uint64_t, uint32_t *> addr_to_reg;

	SC_HAS_PROCESS(CLINT);

	CLINT(sc_core::sc_module_name) {
		tsock.register_b_transport(this, &CLINT::transport);

		addr_to_reg = {
		    {0xBFF8, (uint32_t *)&mtime},
		    {0xBFFC, (uint32_t *)&mtime + 1},
		};

		for (unsigned i=0; i<NumberOfCores; ++i) {
            addr_to_reg[i*4] = (uint32_t *)&msip[i];

			auto off = i*8;
			addr_to_reg[0x4000+off] = (uint32_t *)&mtimecmp[i];
			addr_to_reg[0x4004+off] = ((uint32_t *)&mtimecmp[i]) + 1;
		}

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

			for (unsigned i=0; i<NumberOfCores; ++i) {
				//std::cout << "[vp::clint] process mtimecmp[" << i << "]=" << mtimecmp[i] << ", mtime=" << mtime << std::endl;
				auto cmp = mtimecmp[i];
				if (cmp > 0 && mtime >= cmp) {
					//std::cout << "[vp::clint] set timer interrupt for core " << i << std::endl;
					target_harts[i]->trigger_timer_interrupt(true);
				} else {
					//std::cout << "[vp::clint] unset timer interrupt for core " << i << std::endl;
					target_harts[i]->trigger_timer_interrupt(false);
					if (cmp > 0) {
						auto time = sc_core::sc_time::from_value(mtime * scaler);
						auto goal = sc_core::sc_time::from_value(cmp * scaler);
						irq_event.notify(goal - time);
					}
				}
			}
		}
	}

	void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
		assert (trans.get_data_length() == 4);
		assert (trans.get_address() % 4 == 0);

		auto it = addr_to_reg.find(trans.get_address());
		assert (it != addr_to_reg.end());

		delay += clock_cycle;
		sc_core::sc_time now = sc_core::sc_time_stamp() + delay;

		if (trans.get_command() == tlm::TLM_READ_COMMAND) {
			mtime = now.value() / scaler;

			*((uint32_t *)trans.get_data_ptr()) = *it->second;
		} else if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
		    auto addr = trans.get_address();

			assert (addr != 0xBFF8 && addr != 0xBFFC);  // mtime is readonly

			*it->second = *((uint32_t *)trans.get_data_ptr());

			if (addr < 0x4000) {
			    // write to msip register
			    unsigned idx = addr / 4;
			    msip[idx] &= 0x1;
			    target_harts[idx]->trigger_software_interrupt(msip[idx] != 0);
			} else if (addr >= 0x4000 && addr < 0xBFF8) {
				//std::cout << "[vp::clint] write mtimecmp[addr=" << addr << "]=" << *it->second << ", mtime=" << mtime << std::endl;
                irq_event.notify(delay); // write to mtimecmp, update timer interrupts
			} else {
			    assert (false && "unmapped access");
			}
		} else {
			assert(false && "unsupported tlm command for CLINT access");
		}
	}
};

#endif  // RISCV_ISA_CLINT_H
