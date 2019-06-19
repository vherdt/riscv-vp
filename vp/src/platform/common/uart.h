#ifndef RISCV_VP_UART_H
#define RISCV_VP_UART_H

#include "abstract_uart.h"

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <systemc>

#include <sys/types.h>

class UART : public AbstractUART {
	struct termios orig_termios;

public:
	UART(const sc_core::sc_module_name& name, uint32_t irqsrc)
		: AbstractUART(name, irqsrc)
	{
		if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
			throw std::system_error(errno, std::generic_category());

		struct termios raw = orig_termios;
		raw.c_lflag &= ~(ICANON); // Bytewise read
		raw.c_lflag &= ~(ECHO); // Disable local echo
		raw.c_iflag &= ~(ICRNL); // Don't map CR to NL

		if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
			throw std::system_error(errno, std::generic_category());

		start_threads();
	}

	~UART() {
		// TODO: Don't ignore return value
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
	}

private:
	void write_data(uint8_t data) {
		ssize_t nwritten;

		nwritten = write(STDOUT_FILENO, &data, sizeof(data));
		if (nwritten == -1)
			throw std::system_error(errno, std::generic_category());
		else if (nwritten != sizeof(data))
			throw std::runtime_error("short write");
	}

	void read_data(std::mutex &mtx, std::queue<uint8_t> &queue) {
		uint8_t buf;
		ssize_t nread;

		nread = read(STDIN_FILENO, &buf, sizeof(buf));
		if (nread == -1)
			throw std::system_error(errno, std::generic_category());
		else if (nread != sizeof(buf))
			throw std::runtime_error("short read");

		mtx.lock();
		queue.push(buf);
		mtx.unlock();
	}
};

#endif  // RISCV_VP_UART_H
