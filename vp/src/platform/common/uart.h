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
	typedef enum {
		STATE_COMMAND,
		STATE_NORMAL,
	} uart_state;

	/**
	 * State of the input handling state machine. In normal mode
	 * (STATE_NORMAL) the next input character is forwarded to the
	 * guest. In command mode (STATE_COMMAND) the next input
	 * character is interpreted by ::handle_cmd.
	 */
	uart_state state = STATE_NORMAL;
	void handle_cmd(uint8_t);

	void handle_input(int fd) override;
	void write_data(uint8_t) override;
};

#endif  // RISCV_VP_UART_H
