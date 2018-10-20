#ifndef RISCV_VP_SPI_H
#define RISCV_VP_SPI_H

#include <systemc>

#include <tlm_utils/simple_target_socket.h>

#include "tlm_map.h"
#include "irq_if.h"


struct SPI : public sc_core::sc_module {
    tlm_utils::simple_target_socket<SPI> tsock;

    // memory mapped configuration registers
    uint32_t sckdiv = 0;

    enum {
        SCKDIV_REG_ADDR = 0x0,
    };

    vp::map::LocalRouter router;

    SPI(sc_core::sc_module_name) {
        tsock.register_b_transport(this, &SPI::transport);

        router.add_register_bank({
                 {SCKDIV_REG_ADDR, &sckdiv},
         }).register_handler(this, &SPI::register_access_callback);
    }

    void register_access_callback(const vp::map::register_access_t &r) {
        r.fn();
    }

    void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
        router.transport(trans, delay);
    }
};

#endif //RISCV_VP_SPI_H
