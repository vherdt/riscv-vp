#pragma once

#include "spi.h"
#include "can/mcp_can_dfs.h"

#include <functional>
#include <thread>

class CAN : public SpiInterface
{
	enum class State
	{
		init,
		readRegister,
		writeRegister,
		bitmod,

		loadTX0,
		loadTX1,
		loadTX2,

		sendTX0,
		sendTX1,
		sendTX2,
		sendALL,

		getStatus,

		shit,
		wank,
		fuck,
		arse,
		crap,
		dick,
	} state ;

	std::thread listener;

	uint8_t registers[MCP_RXB1SIDH+1];

	struct TXBuf {
		union {
			uint8_t raw[32];
			struct {
				uint8_t id;
				uint8_t length;
				uint8_t payload[30];
			} as;
		};
	} txBuf[3];

	uint8_t status;

public:
	CAN();
	~CAN();

	uint8_t write(uint8_t byte) override;

	const char* registerName(uint8_t id);
	const char* regValueName(uint8_t id);
	const char* spiInstName(uint8_t id);

	void command(uint8_t byte);
	uint8_t readRegister(uint8_t byte);
	uint8_t writeRegister(uint8_t byte);
	uint8_t modifyRegister(uint8_t byte);

	uint8_t loadTxBuf(uint8_t no, uint8_t byte);
	uint8_t sendTxBuf(uint8_t no, uint8_t byte);

	void listen();
};
