#ifndef RISCV_ISA_IRQ_IF_H
#define RISCV_ISA_IRQ_IF_H


struct external_interrupt_target {
    virtual ~external_interrupt_target() {}

    virtual void trigger_external_interrupt() = 0;
};

struct timer_interrupt_target {
    virtual ~timer_interrupt_target() {}

    virtual void trigger_timer_interrupt(bool status) = 0;
};

struct interrupt_gateway {
    virtual ~interrupt_gateway() {}

    virtual void gateway_incoming_interrupt(uint32_t irq_id) = 0;
};


#endif //RISCV_ISA_IRQ_IF_H
