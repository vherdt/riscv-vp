#include "irq.h"
#include "assert.h"

	
#define RISCV_MACHINE_SOFTWARE_INTERRUPT 3
#define RISCV_MACHINE_TIMER_INTERRUPT 7
#define RISCV_MACHINE_EXTERNAL_INTERRUPT 11


static volatile uint32_t * const PLIC_CLAIM_AND_RESPONSE_REGISTER = (uint32_t * const)0x40200004;

static void irq_empty_handler() {}

#define IRQ_TABLE_NUM_ENTRIES 64

static irq_handler_t irq_handler_table[IRQ_TABLE_NUM_ENTRIES] = { [ 0 ... IRQ_TABLE_NUM_ENTRIES-1 ] = irq_empty_handler };


#define CLINT_BASE 0x2000000

volatile uint64_t* mtime =   (uint64_t*)(CLINT_BASE + 0xbff8);
volatile uint64_t* mtimecmp = (uint64_t*)(CLINT_BASE + 0x4000);

static irq_handler_t timer_irq_handler = 0;


void level_1_interrupt_handler(uint32_t cause) {

	switch (cause & 0xf) {
		case RISCV_MACHINE_EXTERNAL_INTERRUPT: {
			register long t0 asm("t0") = 0x800;
			asm volatile ("csrc mip, t0");
			
			uint32_t irq_id = *PLIC_CLAIM_AND_RESPONSE_REGISTER;
	
			irq_handler_table[irq_id]();
	
			*PLIC_CLAIM_AND_RESPONSE_REGISTER = 1;
			
			return;
		}
		
		case RISCV_MACHINE_TIMER_INTERRUPT: {
		    // Note: the pending timer interrupt bit will be automatically cleared when writing to the *mtimecmp* register of this hart
            if (timer_irq_handler) {
                // let the user registered handler clear the timer interrupt bit
                timer_irq_handler();
            } else {
                // reset the *mtimecmp* register to zero to clear the pending bit
                *mtimecmp = 0;
            }

		    return;
		}
	}
	
	assert (0 && "unsupported cause");
}

void register_interrupt_handler(uint32_t irq_id, irq_handler_t fn) {
	assert (irq_id < IRQ_TABLE_NUM_ENTRIES);
	irq_handler_table[irq_id] = fn;
}

void register_timer_interrupt_handler(irq_handler_t fn) {
    timer_irq_handler = fn;
}
