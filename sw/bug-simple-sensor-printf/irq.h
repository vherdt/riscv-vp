#ifndef __RISCV_IRQ_H__
#define __RISCV_IRQ_H__

#include "stdint.h"

typedef void (*irq_handler_t)(void);

void register_interrupt_handler(uint32_t irq_id, irq_handler_t fn);

void register_timer_interrupt_handler(irq_handler_t fn);

extern volatile uint64_t* mtime;
extern volatile uint64_t* mtimecmp;

#endif
