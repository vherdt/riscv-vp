#ifndef RISCV_VP_UART_H
#define RISCV_VP_UART_H

#include <systemc>

#include <tlm_utils/simple_target_socket.h>

#include "tlm_map.h"
#include "irq_if.h"

#include <deque>


struct Reg32 {
    uint32_t value;

    Reg32(uint32_t n)
            : value(n){
    }

    Reg32 operator =(uint32_t n) {
        value = n;
    }

    operator uint32_t() const {
        return value;
    }


};


struct Reg32Field {
    Reg32 *owner;
    unsigned start;
    unsigned end;

    Reg32Field(Reg32 *owner, unsigned start, unsigned end)
            : owner(owner), start(start), end(end) {
    }
};


struct UART : public sc_core::sc_module {
    tlm_utils::simple_target_socket<UART> tsock;

    // memory mapped configuration registers
    uint32_t txdata = 0;
    uint32_t rxdata = 0;
    uint32_t txctrl = 0;
    uint32_t rxctrl = 0;
    uint32_t ie = 0;
    uint32_t ip = 0;
    uint32_t div = 0;

    std::deque<uint8_t> tx_fifo;
    std::deque<uint8_t> rx_fifo;

    enum {
        TXDATA_REG_ADDR = 0x0,
        RXDATA_REG_ADDR = 0x4,
        TXCTRL_REG_ADDR = 0x8,
        RXCTRL_REG_ADDR = 0xC,
        IE_REG_ADDR = 0x10,
        IP_REG_ADDR = 0x14,
        DIV_REG_ADDR = 0x18,
    };

    vp::map::LocalRouter router;

    UART(sc_core::sc_module_name) {
        tsock.register_b_transport(this, &UART::transport);

        router.add_register_bank({
                 {TXDATA_REG_ADDR, &txdata},
                 {RXDATA_REG_ADDR, &rxdata},
                 {TXCTRL_REG_ADDR, &txctrl},
                 {RXCTRL_REG_ADDR, &rxctrl},
                 {IE_REG_ADDR, &ie},
                 {IP_REG_ADDR, &ip},
                 {DIV_REG_ADDR, &div},
         }).register_handler(this, &UART::register_access_callback);
    }

    void register_access_callback(const vp::map::register_access_t &r) {
        if (r.read && r.vptr == &rxdata) {
            rxdata &= ~0xff;
            rxdata |= rand() & 0xff;
        }

        r.fn();

        if (r.write && r.vptr == &txdata) {
            uint8_t byte = txdata & 0xff;
            std::cout << (char)byte;
        }
    }

    void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
        router.transport(trans, delay);
    }
};

#endif //RISCV_VP_UART_H
