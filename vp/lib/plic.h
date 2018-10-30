#ifndef RISCV_ISA_IRQCTL_H
#define RISCV_ISA_IRQCTL_H

#include <systemc>
#include <tlm_utils/simple_target_socket.h>

#include <unordered_map>

#include "irq_if.h"
#include "tlm_map.h"


struct PLIC : public sc_core::sc_module, public interrupt_gateway {

    tlm_utils::simple_target_socket<PLIC> tsock;

    external_interrupt_target *target_hart = nullptr;

    enum {
        NUM_INTERRUPTS = 64,

        PENDING_INTERRUPTS_1_ADDR        = 0x01000,
        PENDING_INTERRUPTS_2_ADDR        = 0x01004,

        HART_0_ENABLED_INTERRUPTS_1_ADDR = 0x02000,
        HART_0_ENABLED_INTERRUPTS_2_ADDR = 0x02004,

        HART_0_PRIORITY_THRESHOLD_ADDR 	 = 0x200000,
        HART_0_CLAIM_RESPONSE_ADDR	     = 0x200004,
    };

    // target (hart) specific information
    uint32_t hart_0_enabled_interrupts_1 = -1;
    uint32_t hart_0_enabled_interrupts_2 = -1;

    uint32_t hart_0_priority_threshold = 0;
    uint32_t hart_0_claim_response     = 0;
    bool hart_0_eip = false;

    // shared for all harts (PLIC supports 63 interrupt sources right now, extend if necessary)
    // priority 1 is the lowest, 7 the highest. Zero means do not interrupt.
    uint32_t interrupt_priorities[NUM_INTERRUPTS] = { 0 };

    uint32_t pending_interrupts_1 = 0;
    uint32_t pending_interrupts_2 = 0;


    vp::map::LocalRouter router = {"PLIC"};

    sc_core::sc_event e_run;
    sc_core::sc_time clock_cycle;

    SC_HAS_PROCESS(PLIC);

    PLIC(sc_core::sc_module_name) {
        clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
        tsock.register_b_transport(this, &PLIC::transport);

        auto &regs = router.add_register_bank({
                 {PENDING_INTERRUPTS_1_ADDR, &pending_interrupts_1},
                 {PENDING_INTERRUPTS_2_ADDR, &pending_interrupts_2},
                 {HART_0_ENABLED_INTERRUPTS_1_ADDR, &hart_0_enabled_interrupts_1},
                 {HART_0_ENABLED_INTERRUPTS_2_ADDR, &hart_0_enabled_interrupts_2},
                 {HART_0_CLAIM_RESPONSE_ADDR, &hart_0_claim_response},
                 {HART_0_PRIORITY_THRESHOLD_ADDR, &hart_0_priority_threshold},
         }).register_handler(this, &PLIC::register_access_callback);

        for (unsigned i=0; i<NUM_INTERRUPTS; ++i) {
            interrupt_priorities[i] = 1;
            regs.add_register({i*4, &interrupt_priorities[i], vp::map::read_write, 0b111});
        }

        SC_THREAD(run);
    }


    void gateway_incoming_interrupt(uint32_t irq_id) {
        // NOTE: can use different techniques for each gateway, in this case a simple non queued edge trigger
        assert (irq_id > 0 && irq_id < NUM_INTERRUPTS);

        if (irq_id < 32) {
            pending_interrupts_1 |= 1 << irq_id;
        } else {
            pending_interrupts_2 |= 1 << (irq_id - 32);
        }

        e_run.notify(clock_cycle);
    }


    void clear_pending_interrupt(int irq_id) {
        assert (irq_id >= 0 && irq_id < NUM_INTERRUPTS);    //NOTE: ignore clear of zero interrupt (zero is not available)

        uint32_t *pending = &pending_interrupts_1;

        if (irq_id >= 32) {
            irq_id -= 32;
            pending = &pending_interrupts_2;
        }

        *pending &= ~(1 << irq_id);
    }

    int hart_0_get_next_pending_interrupt(bool consider_threshold) {
        int min_id = 0;
        unsigned max_priority = 0;

        uint32_t irqs[2] = {
                pending_interrupts_1 & hart_0_enabled_interrupts_1,
                pending_interrupts_2 & hart_0_enabled_interrupts_2
        };

        for (int i=1; i<NUM_INTERRUPTS; ++i) {
            if (irqs[i % 32] & (1 << i)) {
                auto prio = interrupt_priorities[i];
                if (prio > 0 && (!consider_threshold || (prio > hart_0_priority_threshold))) {
                    if (prio > max_priority) {
                        max_priority = prio;
                        min_id = i;
                    }
                }
            }
        }

        return min_id;
    }


    void register_access_callback(const vp::map::register_access_t &r) {
        if (r.write && (r.vptr == &pending_interrupts_1 || r.vptr == &pending_interrupts_2)) {
            assert (false && "pending interrupts registers are read only");
            return;
        }

        if (r.read && r.vptr == &hart_0_claim_response) {
            //NOTE: on claim request retrieve return and clear the interrupt with highest priority, priority threshold is ignored at this point
            int min_id = hart_0_get_next_pending_interrupt(false);
            hart_0_claim_response = min_id;    // zero means no more interrupt to claim
            clear_pending_interrupt(min_id);
        }

        r.fn();

        if (r.write && r.vptr == &hart_0_claim_response) {
            //NOTE: on completed response, check if there are any other pending interrupts
            hart_0_eip = false;
            e_run.notify(clock_cycle);
        }
    }

    void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
        router.transport(trans, delay);
    }

    bool hart_0_has_pending_enabled_interrupts() {
        return hart_0_get_next_pending_interrupt(true) > 0;
    }

    void run() {
        while (true) {
            sc_core::wait(e_run);

            //NOTE: has to be done for every hart
            if (!hart_0_eip) {
                if (hart_0_has_pending_enabled_interrupts()) {
                    hart_0_eip = true;
                    target_hart->trigger_external_interrupt();
                }
            }
        }
    }
};


#endif //RISCV_ISA_IRQCTL_H
