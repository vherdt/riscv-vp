#ifndef RISCV_VP_UART_H
#define RISCV_VP_UART_H

#include <stdint.h>
#include <systemc>
#include "abstract_uart.h"

class UART : public AbstractUART {
public:
	UART(const sc_core::sc_module_name&, uint32_t);
	~UART(void);

private:
	void handle_input(int fd) override;
	void write_data(uint8_t) override;
};

#endif  // RISCV_VP_UART_H
