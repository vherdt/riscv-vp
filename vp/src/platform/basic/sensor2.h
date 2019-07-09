#ifndef RISCV_ISA_SENSOR2_H
#define RISCV_ISA_SENSOR2_H

#include <cstdlib>
#include <cstring>

#include "core/common/irq_if.h"
#include "util/tlm_map.h"

/*
 * Provides the same functionality as sensor.h but uses the optional modelling
 * layer provided in util/tlm_map.h.
 */

struct SimpleSensor2 : public sc_core::sc_module {
	tlm_utils::simple_target_socket<SimpleSensor2> tsock;

	interrupt_gateway *plic = 0;
	uint32_t irq_number = 0;
	sc_core::sc_event run_event;

	// memory mapped data frame
	std::array<uint8_t, 64> data_frame;

	// memory mapped configuration registers
	uint32_t scaler = 25;
	uint32_t filter = 0;

	enum {
		SCALER_REG_ADDR = 0x80,
		FILTER_REG_ADDR = 0x84,
	};

	vp::map::LocalRouter router;

	SC_HAS_PROCESS(SimpleSensor2);

	SimpleSensor2(sc_core::sc_module_name, uint32_t irq_number) : irq_number(irq_number) {
		tsock.register_b_transport(this, &SimpleSensor2::transport);
		SC_THREAD(run);

		router
		    .add_register_bank({
		        {SCALER_REG_ADDR, &scaler},
		        {FILTER_REG_ADDR, &filter},
		    })
		    .register_handler(this, &SimpleSensor2::register_access_callback);

		router.add_start_size_mapping(0x00, data_frame.size(), vp::map::read_only)
		    .register_handler(this, &SimpleSensor2::data_frame_access_callback);
	}

	void data_frame_access_callback(tlm::tlm_generic_payload &trans, sc_core::sc_time) {
		// return last generated random data at requested address
		vp::map::execute_memory_access(trans, data_frame.data());
	}

	void register_access_callback(const vp::map::register_access_t &r) {
		// trigger pre read/write actions
		if (r.write && (r.vptr == &scaler)) {
			if (r.nv < 1 || r.nv > 100)
				return;
		}

		// actual read/write
		r.fn();

		// trigger post read/write actions
		if (r.write && (r.vptr == &scaler)) {
			run_event.cancel();
			run_event.notify(sc_core::sc_time(scaler, sc_core::SC_MS));
		}
	}

	void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
		router.transport(trans, delay);
	}

	void run() {
		while (true) {
			run_event.notify(sc_core::sc_time(scaler, sc_core::SC_MS));
			sc_core::wait(run_event);  // 40 times per second by default

			// fill with random data
			for (auto &n : data_frame) {
				if (filter == 1) {
					n = rand() % 10 + 48;
				} else if (filter == 2) {
					n = rand() % 26 + 65;
				} else {
					// fallback for all other filter values
					n = rand();  // random char
				}
			}

			plic->gateway_trigger_interrupt(irq_number);
		}
	}
};

#endif  // RISCV_ISA_SENSOR2_H
