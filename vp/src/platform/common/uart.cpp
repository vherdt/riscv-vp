#include "uart.h"

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <systemc>

#include "core/common/rawmode.h"

UART::UART(const sc_core::sc_module_name& name, uint32_t irqsrc) : AbstractUART(name, irqsrc) {
	enableRawMode(STDIN_FILENO);
	start_threads();
}

UART::~UART(void) {
	disableRawMode(STDIN_FILENO);
}

void UART::write_data(uint8_t data) {
	ssize_t nwritten;

	nwritten = write(STDOUT_FILENO, &data, sizeof(data));
	if (nwritten == -1)
		throw std::system_error(errno, std::generic_category());
	else if (nwritten != sizeof(data))
		throw std::runtime_error("short write");
}

void UART::read_data(void) {
	uint8_t buf;
	ssize_t nread;

	nread = read(STDIN_FILENO, &buf, sizeof(buf));
	if (nread == -1)
		throw std::system_error(errno, std::generic_category());
	else if (nread != sizeof(buf))
		throw std::runtime_error("short read");

	rxpush(buf);
}
