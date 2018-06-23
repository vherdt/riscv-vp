#ifndef RISCV_VP_ETHERNET_H
#define RISCV_VP_ETHERNET_H

#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <ios>
#include <iomanip>

#include <unistd.h>

#include <systemc>

#include <tlm_utils/simple_target_socket.h>

#include "irq_if.h"
#include "tlm_map.h"


struct raw_ethernet_frame {

};


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

    int send_sockfd = 0;
    int recv_sockfd = 0;

    enum {
        MTU_SIZE = 1500,
        FRAME_SIZE = 1514,
    };

    uint8_t recv_payload_buf[MTU_SIZE];
    uint8_t recv_frame_buf[FRAME_SIZE];
    bool has_frame;

    enum {
        STATUS_REG_ADDR = 0x00,
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

        init_raw_sockets();
    }

    void init_raw_sockets();

    void send_raw_frame();
    void try_recv_raw_frame();

    void register_access_callback(const vp::map::register_access_t &r) {
        assert (mem);

        if (r.read && r.vptr == &receive_size) {
            assert (has_frame);
        }

        r.fn();

        if (r.write && r.vptr == &status) {
            if (r.nv == RECV_OPERATION) {
                //std::cout << "[ethernet] recv operation" << std::endl;
                assert (has_frame);
                for (int i=0; i<receive_size; ++i) {
                    auto k = receive_dst + i;
                    mem[k - 0x80000000] = recv_frame_buf[i];
                }
                has_frame = false;
            } else if (r.nv == SEND_OPERATION) {
                send_raw_frame();
                /*
                for (int i=0; i<10; ++i) {
                    usleep(1000000);
                    send_raw_frame();
                }
                 */
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
            run_event.notify(sc_core::sc_time(10000, sc_core::SC_US));
            sc_core::wait(run_event);  // 10000 times per second by default

            // check if data is available on the socket, if yes store it in an internal buffer
            if (!has_frame) {
                try_recv_raw_frame();

                if (has_frame)
                    plic->gateway_incoming_interrupt(irq_number);
            }
        }
    }
};



#endif //RISCV_VP_ETHERNET_H
