#ifndef RISCV_VP_ABSTRACT_UART_H
#define RISCV_VP_ABSTRACT_UART_H

#include <systemc>

#include <tlm_utils/simple_target_socket.h>

#include "core/common/irq_if.h"
#include "util/tlm_map.h"

/* TODO: move this header to common? */
#include <platform/hifive/async_event.h>

#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <queue>
#include <thread>
#include <mutex>

#define UART_TXWM (1 << 0)
#define UART_RXWM (1 << 1)

/* Extracts the interrupt trigger threshold from a control register */
#define UART_CTRL_CNT(REG) ((REG) >> 16)

class AbstractUART : public sc_core::sc_module {
	uint32_t irq;

	// memory mapped configuration registers
	uint32_t txdata = 0;
	uint32_t rxdata = 0;
	uint32_t txctrl = 0;
	uint32_t rxctrl = 0;
	uint32_t ie = 0;
	uint32_t ip = 0;
	uint32_t div = 0;

	std::thread rcvthr, txthr;
	std::mutex rcvmtx, txmtx;
	AsyncEvent asyncEvent;
	sem_t txsem;

	std::queue<uint8_t> tx_fifo;
	std::queue<uint8_t> rx_fifo;

	SC_HAS_PROCESS(AbstractUART);

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

public:
	interrupt_gateway *plic;
	tlm_utils::simple_target_socket<AbstractUART> tsock;

	AbstractUART(sc_core::sc_module_name, uint32_t irqsrc) {
		irq = irqsrc;
		tsock.register_b_transport(this, &AbstractUART::transport);

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
		    .register_handler(this, &AbstractUART::register_access_callback);

		if (sem_init(&txsem, 0, 0))
			throw std::system_error(errno, std::generic_category());

		SC_METHOD(interrupt);
		sensitive << asyncEvent;
		dont_initialize();
	}

	~AbstractUART() {
		// TODO: kill threads
		// TODO: destroy semaphore
	}

protected:
	void start_threads(void) {
		rcvthr = std::thread(&AbstractUART::receive, this);
		rcvthr.detach();

		txthr = std::thread(&AbstractUART::transmit, this);
		txthr.detach();
	}

private:
	virtual void write_data(uint8_t) = 0;
	virtual uint8_t read_data(void) = 0;

	void register_access_callback(const vp::map::register_access_t &r) {
		if (r.read) {
			if (r.vptr == &txdata) {
				txdata = 0;  // always transmit
			} else if (r.vptr == &rxdata) {
				rcvmtx.lock();
				if (rx_fifo.empty()) {
					rxdata = 1 << 31;
				} else {
					rxdata = rx_fifo.front();
					rx_fifo.pop();
				}

				if (rx_fifo.empty() || rx_fifo.size() < UART_CTRL_CNT(rxctrl))
					ip &= ~UART_RXWM;
				rcvmtx.unlock();
			} else if (r.vptr == &txctrl) {
				// std::cout << "TXctl";
			} else if (r.vptr == &rxctrl) {
				// std::cout << "RXctrl";
			} else if (r.vptr == &ie || r.vptr == &ip) {
				/* ie = 0;  // no interrupts enabled */
				/* ip = 0;  // no interrupts pending */
			} else if (r.vptr == &div) {
				// just return the last set value
			} else {
				std::cerr << "invalid offset for UART " << std::endl;
			}
		}

		bool notify = false;
		if (r.write) {
			if (r.vptr == &txctrl && UART_CTRL_CNT(r.nv) < UART_CTRL_CNT(txctrl))
				notify = true;
			else if (r.vptr == &rxctrl && UART_CTRL_CNT(r.nv) < UART_CTRL_CNT(rxctrl))
				notify = true;
		}

		r.fn();

		if (notify || (r.write && r.vptr == &ie))
			asyncEvent.notify();

		if (r.write && r.vptr == &txdata) {
			txmtx.lock();
			tx_fifo.push(txdata);
			if (sem_post(&txsem))
				throw std::system_error(errno, std::generic_category());
			if (tx_fifo.size() > UART_CTRL_CNT(txctrl))
				ip &= ~UART_TXWM;
			txmtx.unlock();
		}
	}

	void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
		router.transport(trans, delay);
	}

	void transmit(void) {
		uint8_t data;

		for (;;) {
			if (sem_wait(&txsem))
				throw std::system_error(errno, std::generic_category());

			txmtx.lock();
			data = tx_fifo.front();
			tx_fifo.pop();
			txmtx.unlock();
			asyncEvent.notify();

			write_data(data);
		}
	}

	void receive(void) {
		uint8_t data;

		for (;;) {
			data = read_data();
		
			rcvmtx.lock();
			rx_fifo.push(data);
			rcvmtx.unlock();
			asyncEvent.notify();
		}
	}

	void interrupt(void) {
		bool trigger;

		/* XXX: Possible optimization would be to trigger the
		 * interrupt from the background thread. However,
		 * the PLIC methods are very likely not thread safe. */

		if (ie & UART_RXWM) {
			rcvmtx.lock();
			if (rx_fifo.size() > UART_CTRL_CNT(rxctrl)) {
				ip |= UART_RXWM;
				trigger = true;
			}
			rcvmtx.unlock();
		}

		if (ie & UART_TXWM) {
			txmtx.lock();
			if (UART_CTRL_CNT(txctrl) == 0 || tx_fifo.size() < UART_CTRL_CNT(txctrl)) {
				ip |= UART_TXWM;
				trigger = true;
			}
			txmtx.unlock();
		}

		if (trigger)
			plic->gateway_trigger_interrupt(irq);
	}
};

#endif  // RISCV_VP_ABSTRACT_UART_H
