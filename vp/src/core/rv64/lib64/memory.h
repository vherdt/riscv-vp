#ifndef RISCV_ISA_MEMORY_H
#define RISCV_ISA_MEMORY_H

#include <stdint.h>

#include <iostream>

#include "bus.h"

#include "systemc"

#include "tlm_utils/simple_target_socket.h"


struct SimpleMemory : public sc_core::sc_module {
    tlm_utils::simple_target_socket<SimpleMemory> tsock;

    uint8_t *data;
    uint32_t size;

    SimpleMemory(sc_core::sc_module_name, uint32_t size)
        : data(new uint8_t[size]()), size(size) {
        tsock.register_b_transport(this, &SimpleMemory::transport);
        tsock.register_get_direct_mem_ptr(this, &SimpleMemory::get_direct_mem_ptr);
    }

    void write_data(unsigned addr, uint8_t *src, unsigned num_bytes) {
        assert (addr + num_bytes <= size);

        memcpy(data + addr, src, num_bytes);
    }

    void read_data(unsigned addr, uint8_t *dst, unsigned num_bytes) {
        assert (addr + num_bytes <= size);

        memcpy(dst, data + addr, num_bytes);
    }

    void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
        tlm::tlm_command cmd = trans.get_command();
        unsigned addr = trans.get_address();
        auto *ptr = trans.get_data_ptr();
        auto len = trans.get_data_length();

        assert ((addr >= 0) && (addr < size));

        if(cmd == tlm::TLM_WRITE_COMMAND) {
            write_data(addr, ptr, len);
        } else if (cmd == tlm::TLM_READ_COMMAND) {
            read_data(addr, ptr, len);
        } else {
            sc_assert (false && "unsupported tlm command");
        }

        delay += sc_core::sc_time(10, sc_core::SC_NS);
    }

    bool get_direct_mem_ptr(tlm::tlm_generic_payload &trans, tlm::tlm_dmi &dmi) {
        dmi.allow_read_write();
        dmi.set_start_address(0);
        dmi.set_end_address(size);
        dmi.set_dmi_ptr(data);
        return true;
    }
};


#endif //RISCV_ISA_MEMORY_H
