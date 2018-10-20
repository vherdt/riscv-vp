#ifndef RISCV_VP_AON_H
#define RISCV_VP_AON_H

#include <systemc>

#include <tlm_utils/simple_target_socket.h>

#include "irq_if.h"


struct AON : public sc_core::sc_module {
    tlm_utils::simple_target_socket<AON> tsock;

    enum {
        LFROSCCFG_REG_ADDR = 0x70,
    };

    AON(sc_core::sc_module_name) {
        tsock.register_b_transport(this, &AON::transport);
    }

    void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
        auto addr = trans.get_address();
        auto cmd  = trans.get_command();
        auto len  = trans.get_data_length();
        auto ptr  = trans.get_data_ptr();

        if (addr == LFROSCCFG_REG_ADDR) {
            // ignore for now
        } else {
            assert (false);
        }
    }
};

#endif //RISCV_VP_AON_H
