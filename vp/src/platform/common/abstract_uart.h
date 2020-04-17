#ifndef RISCV_VP_ABSTRACT_UART_H
#define RISCV_VP_ABSTRACT_UART_H

#include <systemc>

#include <tlm_utils/simple_target_socket.h>

#include "core/common/irq_if.h"
#include "util/tlm_map.h"
#include "platform/common/async_event.h"

#include <fcntl.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdlib.h>
#include <mutex>
#include <queue>
#include <thread>

#define UART_TXWM (1 << 0)
#define UART_RXWM (1 << 1)
#define UART_FULL (1 << 31)

/* 8-entry transmit and receive FIFO buffers */
#define UART_FIFO_DEPTH 8

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

	std::queue<uint8_t> tx_fifo;
	sem_t txfull;
	std::queue<uint8_t> rx_fifo;
	sem_t rxempty;

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

		if (sem_init(&txfull, 0, 0))
			throw std::system_error(errno, std::generic_category());
		if (sem_init(&rxempty, 0, UART_FIFO_DEPTH))
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

	void rxpush(uint8_t data) {
		swait(&rxempty);
		rcvmtx.lock();
		rx_fifo.push(data);
		rcvmtx.unlock();
		asyncEvent.notify();
	}

   private:
	virtual void write_data(uint8_t) = 0;
	virtual void read_data(void) = 0;

	void register_access_callback(const vp::map::register_access_t &r) {
		if (r.read) {
			if (r.vptr == &txdata) {
				txmtx.lock();
				txdata = (tx_fifo.size() >= UART_FIFO_DEPTH) ? UART_FULL : 0;
				txmtx.unlock();
			} else if (r.vptr == &rxdata) {
				rcvmtx.lock();
				if (rx_fifo.empty()) {
					rxdata = 1 << 31;
				} else {
					rxdata = rx_fifo.front();
					rx_fifo.pop();
					spost(&rxempty);
				}

				rcvmtx.unlock();
			} else if (r.vptr == &txctrl) {
				// std::cout << "TXctl";
			} else if (r.vptr == &rxctrl) {
				// std::cout << "RXctrl";
			} else if (r.vptr == &ip) {
				uint32_t ret = 0;
				txmtx.lock();
				if (tx_fifo.size() < UART_CTRL_CNT(txctrl)) {
					ret |= UART_TXWM;
				}
				txmtx.unlock();
				rcvmtx.lock();
				if (rx_fifo.size() > UART_CTRL_CNT(rxctrl)) {
					ret |= UART_RXWM;
				}
				rcvmtx.unlock();
				ip = ret;
			} else if (r.vptr == &ie) {
				// do nothing
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
			if (tx_fifo.size() >= UART_FIFO_DEPTH) {
				txmtx.unlock();
				return; /* write is ignored */
			}

			tx_fifo.push(txdata);
			txmtx.unlock();
			spost(&txfull);
		}
	}

	void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
		router.transport(trans, delay);
	}

	void transmit(void) {
		uint8_t data;

		for (;;) {
			swait(&txfull);
			txmtx.lock();
			data = tx_fifo.front();
			tx_fifo.pop();
			txmtx.unlock();

			asyncEvent.notify();
			write_data(data);
		}
	}

	void receive(void) {
		for (;;) {
			read_data();
		}
	}

	void interrupt(void) {
		bool trigger = false;

		/* XXX: Possible optimization would be to trigger the
		 * interrupt from the background thread. However,
		 * the PLIC methods are very likely not thread safe. */

		if (ie & UART_RXWM) {
			rcvmtx.lock();
			if (rx_fifo.size() > UART_CTRL_CNT(rxctrl))
				trigger = true;
			rcvmtx.unlock();
		}

		if (ie & UART_TXWM) {
			txmtx.lock();
			if (tx_fifo.size() < UART_CTRL_CNT(txctrl))
				trigger = true;
			txmtx.unlock();
		}

		if (trigger)
			plic->gateway_trigger_interrupt(irq);
	}

	void swait(sem_t *sem) {
		if (sem_wait(sem))
			throw std::system_error(errno, std::generic_category());
	}

	void spost(sem_t *sem) {
		if (sem_post(sem))
			throw std::system_error(errno, std::generic_category());
	}
};

#endif  // RISCV_VP_ABSTRACT_UART_H
