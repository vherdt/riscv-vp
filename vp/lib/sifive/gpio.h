#ifndef RISCV_VP_GPIO_H
#define RISCV_VP_GPIO_H

#include <systemc>

#include <tlm_utils/simple_target_socket.h>

#include "tlm_map.h"
#include "irq_if.h"


struct GPIO : public sc_core::sc_module {
    tlm_utils::simple_target_socket<GPIO> tsock;

    // memory mapped configuration registers
    uint32_t output_en = 0;
    uint32_t port = 0;
    uint32_t iof_en = 0;
    uint32_t iof_sel = 0;
    uint32_t out_xor = 0;

    enum {
        OUTPUT_EN_REG_ADDR = 0x008,
        PORT_REG_ADDR    = 0x00C,
        IOF_EN_REG_ADDR  = 0x038,
        IOF_SEL_REG_ADDR = 0x03C,
        OUT_XOR_REG_ADDR = 0x040,
    };

    vp::map::LocalRouter router = {"GPIO"};

    GPIO(sc_core::sc_module_name) {
        tsock.register_b_transport(this, &GPIO::transport);

        router.add_register_bank({
                 {OUTPUT_EN_REG_ADDR, &output_en},
                 {PORT_REG_ADDR, &port},
                 {IOF_EN_REG_ADDR, &iof_en},
                 {IOF_SEL_REG_ADDR, &iof_sel},
                 {OUT_XOR_REG_ADDR, &out_xor},
         }).register_handler(this, &GPIO::register_access_callback);
    }

    void register_access_callback(const vp::map::register_access_t &r) {
        r.fn();
    }

    void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
        router.transport(trans, delay);
    }
};

#endif //RISCV_VP_GPIO_H
