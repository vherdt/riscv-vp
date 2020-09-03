#include "uart.h"
#include "core/common/rawmode.h"

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <systemc>

#include <sys/types.h>

#define stop_fd (stop_pipe[0])
#define newpollfd(FD) \
	(struct pollfd){.fd = FD, .events = POLLIN | POLLERR};

UART::UART(const sc_core::sc_module_name& name, uint32_t irqsrc)
		: AbstractUART(name, irqsrc) {
	fds[0] = newpollfd(stop_fd);
	fds[1] = newpollfd(STDIN_FILENO);

	enableRawMode(STDIN_FILENO);
	start_threads();
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

void UART::read_data(void) {
	if (poll(fds, (nfds_t)NFDS, -1) == -1)
		throw std::system_error(errno, std::generic_category());

	/* stop_fd is checked first as it is fds[0] */
	for (size_t i = 0; i < NFDS; i++) {
		int fd = fds[i].fd;
		short ev = fds[i].revents;

		if (fd == stop_fd)
			break;
		else if (ev & POLLERR)
			throw std::runtime_error("received unexpected POLLERR");
		else
			handle_input(fd);
	}
}
