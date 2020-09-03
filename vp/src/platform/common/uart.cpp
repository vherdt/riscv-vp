#include "uart.h"
#include "core/common/rawmode.h"

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <systemc>

#include <sys/types.h>

UART::UART(const sc_core::sc_module_name& name, uint32_t irqsrc)
		: AbstractUART(name, irqsrc) {
	enableRawMode(STDIN_FILENO);
	start_threads(STDIN_FILENO);
}

UART::~UART(void) {
	disableRawMode(STDIN_FILENO);
}

void UART::handle_input(int fd) {
	uint8_t buf;
	ssize_t nread;

	nread = read(fd, &buf, sizeof(buf));
	if (nread == -1)
		throw std::system_error(errno, std::generic_category());
	else if (nread != sizeof(buf))
		throw std::runtime_error("short read");

	rxpush(buf);
}

void UART::write_data(uint8_t data) {
	ssize_t nwritten;

	nwritten = write(STDOUT_FILENO, &data, sizeof(data));
	if (nwritten == -1)
		throw std::system_error(errno, std::generic_category());
	else if (nwritten != sizeof(data))
		throw std::runtime_error("short write");
}
