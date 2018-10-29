#ifndef RISCV_ISA_IRQCTL_H
#define RISCV_ISA_IRQCTL_H

#include <unordered_map>

#include <systemc>

#include <tlm_utils/simple_target_socket.h>

#include "irq_if.h"


struct PLIC : public sc_core::sc_module, public interrupt_gateway {

    enum {
        Addr_Target_0_Priority_Threshold    = 0,
        Addr_Target_0_Claim_Response        = 4
    };

    tlm_utils::simple_target_socket<PLIC> tsock;

    external_interrupt_target *target_hart = nullptr;

    std::unordered_map<uint32_t, uint32_t> interrupt_priorities;    // maps id (lower higher priority, 0=no interrupt) to priority (larger higher priority, 0=never interrupt)

    // target (hart) specific information
    uint64_t enabled_mask = -1;
    uint32_t priority_threshold = 0;
    bool eip = false;

    // shared for all harts
    uint64_t pending_interrupts = 0;


    sc_core::sc_event e_run;
    sc_core::sc_time clock_cycle;

    SC_HAS_PROCESS(PLIC);

    PLIC(sc_core::sc_module_name) {
        clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
        tsock.register_b_transport(this, &PLIC::transport);

        SC_THREAD(run);
    }

    void gateway_incoming_interrupt(uint32_t irq_id) {
        // NOTE: can use different techniques for each gateway, in this case a simple non queued edge trigger
        pending_interrupts |= 1 << irq_id;
        e_run.notify(clock_cycle);
    }

    void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
        if (trans.get_address() == Addr_Target_0_Claim_Response) {
            if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
                //NOTE: on completed response, check if there are any other pending interrupts
                eip = false;
                e_run.notify(clock_cycle);
            } else if (trans.get_command() == tlm::TLM_READ_COMMAND) {
                //NOTE: on claim request retrieve return and clear the interrupt with highest priority
                assert (trans.get_data_length() == 4);      // assume 32 bit register size
                auto irqs = pending_interrupts & enabled_mask;
                int min_id = 0;
                for (int i=0; i<irqs; ++i) {
                    if (irqs & (1 << i)) {
                        pending_interrupts &= ~(1 << i);    // clear claimed pending interrupt
                        min_id = i;
                        break;
                    }
                }
                *((uint32_t*)trans.get_data_ptr()) = min_id;    // zero means no more interrupt to claim
            } else {
                assert (false && "unsupported tlm command");
            }
        } else {
        	std::cerr << "access of unmapped address at " << std::hex << trans.get_address() << std::endl;
            assert (false);
        }
    }

    void run() {
        while (true) {
            sc_core::wait(e_run);

            //NOTE: has to be done for every hart
            if (!eip) {
                auto irqs = pending_interrupts & enabled_mask;
                if (irqs != 0) {
                    eip = true;
                    target_hart->trigger_external_interrupt();   //TODO: check for priority threshold and order by priorities
                }
            }
        }
    }
};


#endif //RISCV_ISA_IRQCTL_H
