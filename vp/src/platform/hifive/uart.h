#ifndef RISCV_VP_UART_H
#define RISCV_VP_UART_H

#include <systemc>

#include <tlm_utils/simple_target_socket.h>

#include "core/common/irq_if.h"
#include "util/tlm_map.h"

#include <fcntl.h>
#include <termios.h>
#include <deque>

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

	vp::map::LocalRouter router = {"UART"};

	struct termios orig_termios;

	UART(sc_core::sc_module_name) {
		tsock.register_b_transport(this, &UART::transport);

		router
		    .add_register_bank({
		        {TXDATA_REG_ADDR, &txdata},
		        {RXDATA_REG_ADDR, &rxdata},
		        {TXCTRL_REG_ADDR, &txctrl},
		        {RXCTRL_REG_ADDR, &rxctrl},
		        {IE_REG_ADDR, &ie},
		        {IP_REG_ADDR, &ip},
		        {DIV_REG_ADDR, &div},
		    })
		    .register_handler(this, &UART::register_access_callback);

		int flags = fcntl(0, F_GETFL, 0);
		fcntl(0, F_SETFL, flags | O_NONBLOCK);
		tcgetattr(STDIN_FILENO, &orig_termios);
		struct termios raw = orig_termios;
		raw.c_lflag &= ~(ICANON);  // Bytewise read
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
	}

	~UART() {
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
	}

	void register_access_callback(const vp::map::register_access_t &r) {
		if (r.read) {
			if (r.vptr == &txdata) {
				txdata = 0;  // always transmit
			} else if (r.vptr == &rxdata) {
				char c;
				if (read(0, &c, 1) >= 0) {
					rxdata = c;
				} else {  // rx-queue empty
					rxdata = 1 << 31;
				}
			} else if (r.vptr == &txctrl) {
				// std::cout << "TXctl";
			} else if (r.vptr == &rxctrl) {
				// std::cout << "RXctrl";
			} else if (r.vptr == &ie || r.vptr == &ip) {
				ie = 0;  // no interrupts enabled
				ip = 0;  // no interrupts pending
			} else if (r.vptr == &div) {
				// just return the last set value
			} else {
				std::cerr << "invalid offset for UART " << std::endl;
			}
		}

		r.fn();

		if (r.write && r.vptr == &txdata) {
			std::cout << static_cast<char>(txdata & 0xff);
			fflush(stdout);
		}
	}

	void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
		router.transport(trans, delay);
	}
};

#endif  // RISCV_VP_UART_H
