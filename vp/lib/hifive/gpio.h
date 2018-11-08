#ifndef RISCV_VP_GPIO_H
#define RISCV_VP_GPIO_H

#include <systemc>
#include <tlm_utils/simple_target_socket.h>
#include <functional>
#include <thread>

#include "tlm_map.h"
#include "irq_if.h"

#include "gpio/gpio-server.hpp"

struct GPIO : public sc_core::sc_module {
    tlm_utils::simple_target_socket<GPIO> tsock;

    // memory mapped configuration registers
    uint32_t input_en = 0;
    uint32_t output_en = 0;
    //uint32_t port = 0;
    uint32_t pullup_en = 0;
    uint32_t pin_drive_strength = 0;
    uint32_t rise_intr_en = 0;
    uint32_t rise_intr_pending = 0;
    uint32_t fall_intr_en = 0;
    uint32_t fall_intr_pending = 0;
    uint32_t high_intr_en = 0;
    uint32_t high_intr_pending = 0;
    uint32_t low_intr_en = 0;
    uint32_t low_intr_pending = 0;
    uint32_t iof_en = 0;
    uint32_t iof_sel = 0;
    uint32_t out_xor = 0;

    enum {
    	PIN_VALUE_ADDR	 = 0x000,
    	INPUT_EN_REG_ADDR  = 0x004,
    	OUTPUT_EN_REG_ADDR = 0x008,
        PORT_REG_ADDR    = 0x00C,
        PULLUP_EN_ADDR   = 0x010,
        PIN_DRIVE_STNGTH = 0x014,
        RISE_INTR_EN	 = 0x018,
        RISE_INTR_PEND	 = 0x01C,
        FALL_INTR_EN	 = 0x020,
        FALL_INTR_PEND	 = 0x024,
        HIGH_INTR_EN	 = 0x028,
        HIGH_INTR_PEND	 = 0x02C,
        LOW_INTR_EN		 = 0x030,
        LOW_INTR_PEND	 = 0x034,
        IOF_EN_REG_ADDR  = 0x038,
        IOF_SEL_REG_ADDR = 0x03C,
        OUT_XOR_REG_ADDR = 0x040,
    };

    vp::map::LocalRouter router = {"GPIO"};
    interrupt_gateway *plic = nullptr;

    SC_HAS_PROCESS(GPIO);

    GpioServer server;
    std::thread serverThread;

    GPIO(sc_core::sc_module_name) {
        tsock.register_b_transport(this, &GPIO::transport);

        router.add_register_bank({
								{PIN_VALUE_ADDR, reinterpret_cast<uint32_t*>(&server.state)},
								{INPUT_EN_REG_ADDR, &input_en},
								{OUTPUT_EN_REG_ADDR, &output_en},
								{PORT_REG_ADDR, reinterpret_cast<uint32_t*>(&server.state)},
								{PULLUP_EN_ADDR, &pullup_en},
								{PIN_DRIVE_STNGTH, &pin_drive_strength},
								{RISE_INTR_EN, &rise_intr_en},
								{RISE_INTR_PEND, &rise_intr_pending},
								{FALL_INTR_EN, &fall_intr_en},
								{FALL_INTR_PEND, &fall_intr_pending},
								{HIGH_INTR_EN, &high_intr_en},
								{HIGH_INTR_PEND, &high_intr_pending},
								{LOW_INTR_EN, &low_intr_en},
								{LOW_INTR_PEND, &low_intr_pending},
								{IOF_EN_REG_ADDR, &iof_en},
								{IOF_SEL_REG_ADDR, &iof_sel},
								{OUT_XOR_REG_ADDR, &out_xor},
         }).register_handler(this, &GPIO::register_access_callback);

        server.setupConnection("1339");

        serverThread = std::thread(std::bind(&GpioServer::startListening, &server));
        SC_THREAD(run);
    }

    ~GPIO()
    {
    	server.quit();
    	serverThread.join();
    }

    void register_access_callback(const vp::map::register_access_t &r) {
        r.fn();
        if(r.write)
        {
        	//std::cout << "Write to GPIO reg. no " << (r.vptr - &pin_value);
			if(r.vptr == reinterpret_cast<uint32_t*>(&server.state))
			{
				std::cout << "[VP] new GPIO Value: ";
				for(unsigned i = 0; i < sizeof(uint32_t) * 8; i++)
				{
					if(i > 1 && (i % 8 == 0))
						std::cout << " ";
					printf("%c", server.state & 1 << (32 - i) ? '1' : '0');
				}
			}
			std::cout << std::endl;
        }
    }

    void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
        router.transport(trans, delay);
    }

    void run() {
    	sc_core::sc_event run_event;
        while (true) {
            run_event.notify(sc_core::sc_time(10000, sc_core::SC_US));
            sc_core::wait(run_event);  // 10000 times per second by default
            //Todo: wait for pin input from external sources
            //std::cout << "Hi " << sc_core::sc_time_stamp() << std::endl;
        }
    }
};

#endif //RISCV_VP_GPIO_H
