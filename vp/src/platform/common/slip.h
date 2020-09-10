#ifndef RISCV_VP_SLIP_H
#define RISCV_VP_SLIP_H

#include <stdint.h>
#include <systemc>
#include "abstract_uart.h"

class SLIP : public AbstractUART {
public:
	SLIP(const sc_core::sc_module_name &, uint32_t, std::string);
	~SLIP(void);

private:
	int get_mtu(const char *);
	void send_packet(void);
	void write_data(uint8_t) override;
	void handle_input(int fd) override;

	int tunfd;

	uint8_t *sndbuf = NULL, *rcvbuf = NULL;
	size_t sndsiz, rcvsiz;

};

#endif  // RISCV_VP_SLIP_H
