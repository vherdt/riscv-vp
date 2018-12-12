#ifndef RISCV_ISA_BUS_H
#define RISCV_ISA_BUS_H

#include <map>
#include <stdexcept>

#include <systemc>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>


struct PortMapping {
    uint64_t start;
    uint64_t end;

    PortMapping(uint64_t start, uint64_t end)
            : start(start), end(end) {
        assert (end > start);
    }

    bool contains(uint64_t addr) {
        return addr >= start && addr < end;
    }

    uint64_t global_to_local(uint64_t addr) {
        return addr - start;
    }
};



template <unsigned int NR_OF_INITIATORS, unsigned int NR_OF_TARGETS>
struct SimpleBus : sc_core::sc_module {
    std::array<tlm_utils::simple_target_socket<SimpleBus>, NR_OF_INITIATORS> tsocks;

    std::array<tlm_utils::simple_initiator_socket<SimpleBus>, NR_OF_TARGETS> isocks;
    std::array<PortMapping *, NR_OF_TARGETS> ports;

    SimpleBus(sc_core::sc_module_name) {
        for (auto &s : tsocks)
            s.register_b_transport(this, &SimpleBus::transport);
    }

    unsigned decode(uint64_t addr) {
        for (unsigned i = 0; i < NR_OF_TARGETS; ++i) {
            if (ports[i]->contains(addr))
                return i;
        }
        throw std::runtime_error("unable to find target port for address " + std::to_string(addr));
    }

    void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
        auto addr = trans.get_address();
        auto id = decode(addr);

        trans.set_address(ports[id]->global_to_local(addr));
        isocks[id]->b_transport(trans, delay);
    }
};



#endif //RISCV_ISA_BUS_H
