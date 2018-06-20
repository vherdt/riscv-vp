#ifndef RISCV_VP_ETHERNET_H
#define RISCV_VP_ETHERNET_H

#include <cstdlib>
#include <cstring>
#include <unordered_map>

#include <systemc>

#include <tlm_utils/simple_target_socket.h>

#include "irq_if.h"
#include "tlm_map.h"


struct EthernetDevice : public sc_core::sc_module {
    tlm_utils::simple_target_socket<EthernetDevice> tsock;

    interrupt_gateway *plic = 0;
    uint32_t irq_number = 0;
    sc_core::sc_event run_event;

    // memory mapped data frame
    std::array<uint8_t, 64> data_frame;

    // memory mapped configuration registers
    uint32_t status = 0;
    uint32_t receive_size = 25;
    uint32_t receive_dst = 0;
    uint32_t send_src = 0;
    uint32_t send_size = 0;

    uint8_t *mem = nullptr;

    vp::map::LocalRouter router;

    enum {
        STATUS_REG_ADDR = 0x04,
        RECEIVE_SIZE_REG_ADDR = 0x04,
        RECEIVE_DST_REG_ADDR = 0x08,
        SEND_SRC_REG_ADDR = 0x0c,
        SEND_SIZE_REG_ADDR = 0x10,

        RECV_OPERATION = 1,
        SEND_OPERATION = 2,
    };

    SC_HAS_PROCESS(EthernetDevice);

    EthernetDevice(sc_core::sc_module_name, uint32_t irq_number, uint8_t *mem)
            : irq_number(irq_number), mem(mem) {
        tsock.register_b_transport(this, &EthernetDevice::transport);
        SC_THREAD(run);

        router.add_register_bank({
                 {STATUS_REG_ADDR, &status},
                 {RECEIVE_SIZE_REG_ADDR, &receive_size},
                 {RECEIVE_DST_REG_ADDR, &receive_dst},
                 {SEND_SRC_REG_ADDR, &send_src},
                 {SEND_SIZE_REG_ADDR, &send_size},
         }).register_handler(this, &EthernetDevice::register_access_callback);
    }

    void register_access_callback(const vp::map::register_access_t &r) {
        assert (mem);

        r.fn();

        if (r.write && r.vptr == &status) {
            if (r.nv == RECV_OPERATION) {

            } else if (r.nv == SEND_OPERATION) {
                char buf[send_size];
                memcpy(buf, &mem[send_src], send_size);
                std::cout << "send_size: " << send_size << std::endl;
                for (int i=0; i<send_size; ++i) {
                    std::cout << buf[i];
                }
                std::cout << std::endl;
            } else {
                throw std::runtime_error("unsupported operation");
            }
        }
    }

    void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
        router.transport(trans, delay);
    }

    void run() {
        while (true) {
            run_event.notify(sc_core::sc_time(100, sc_core::SC_US));
            sc_core::wait(run_event);  // 10000 times per second by default

            // check if data is available on the socket, if yes store it in an internal buffer


            plic->gateway_incoming_interrupt(irq_number);
        }
    }
};



#endif //RISCV_VP_ETHERNET_H
