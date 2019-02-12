#pragma once

#include <tlm_utils/simple_target_socket.h>
#include <systemc>

#include "core/common/irq_if.h"
#include "util/tlm_map.h"

struct PLIC : public sc_core::sc_module, public interrupt_gateway {
  tlm_utils::simple_target_socket<PLIC> tsock;

  external_interrupt_target *target_hart = nullptr;

  enum {
    NUM_INTERRUPTS = 64,

    PENDING_INTERRUPTS_1_ADDR = 0x01000,
    PENDING_INTERRUPTS_2_ADDR = 0x01004,

    HART_0_ENABLED_INTERRUPTS_1_ADDR = 0x02000,
    HART_0_ENABLED_INTERRUPTS_2_ADDR = 0x02004,

    HART_0_PRIORITY_THRESHOLD_ADDR = 0x200000,
    HART_0_CLAIM_RESPONSE_ADDR = 0x200004,
  };

  // target (hart) specific information
  uint32_t hart_0_enabled_interrupts_1 = -1;
  uint32_t hart_0_enabled_interrupts_2 = -1;

  uint32_t hart_0_priority_threshold = 0;
  uint32_t hart_0_claim_response = 0;
  bool hart_0_eip = false;

  // shared for all harts (PLIC supports 63 interrupt sources right now, extend if necessary)
  // priority 1 is the lowest, 7 the highest. Zero means do not interrupt.
  uint32_t interrupt_priorities[NUM_INTERRUPTS] = {0};

  uint32_t pending_interrupts_1 = 0;
  uint32_t pending_interrupts_2 = 0;

  vp::map::LocalRouter router = {"PLIC"};

  sc_core::sc_event e_run;
  sc_core::sc_time clock_cycle;

  SC_HAS_PROCESS(PLIC);

  PLIC(sc_core::sc_module_name);

  void gateway_incoming_interrupt(uint32_t irq_id);
  void clear_pending_interrupt(int irq_id);
  int hart_0_get_next_pending_interrupt(bool consider_threshold);
  void register_access_callback(const vp::map::register_access_t &r);
  void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay);
  bool hart_0_has_pending_enabled_interrupts();
  void run();
};
