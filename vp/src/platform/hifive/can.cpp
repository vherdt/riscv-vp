#include "can.h"
#include "can/mcp_can_dfs.h"
#include <unistd.h>

CAN::CAN()
{
	state = State::init;
	listener = std::thread(&CAN::listen, this);
}

CAN::~CAN()
{
	if(listener.joinable())
		listener.join();
}

void CAN::command(uint8_t byte)
{
	std::cout << "[CAN] ";
		switch(byte)
		{
		case MCP_WRITE:
			std::cout << "MCP_WRITE";
			break;
		case MCP_READ:
			std::cout << "MCP_READ";
			state = State::readRegister;
			break;
		case MCP_BITMOD:
			std::cout << "MCP_BITMOD";
			break;
		case MCP_LOAD_TX0:
			std::cout << "MCP_LOAD_TX0";
			break;
		case MCP_LOAD_TX1:
			std::cout << "MCP_LOAD_TX1";
			break;
		case MCP_LOAD_TX2:
			std::cout << "MCP_LOAD_TX2";
			break;
		case MCP_RTS_TX0:
			std::cout << "MCP_RTS_TX0";
			break;
		case MCP_RTS_TX1:
			std::cout << "MCP_RTS_TX1";
			break;
		case MCP_RTS_TX2:
			std::cout << "MCP_RTS_TX2";
			break;
		case MCP_READ_RX0:
			std::cout << "MCP_READ_RX0";
			break;
		case MCP_READ_RX1:
			std::cout << "MCP_READ_RX1";
			break;
		case MCP_READ_STATUS:
			std::cout << "MCP_READ_STATUS";
			break;
		case MCP_RX_STATUS:
			std::cout << "MCP_RX_STATUS";
			break;
		case MCP_RESET:
			std::cout << "MCP_RESET";
			break;

		default:
			std::cout << "unknown";
		}
		std::cout << std::endl;
}


uint8_t CAN::write(uint8_t byte)
{
	switch(state)
	{
	case State::readRegister:
		break;
	case State::init:
		command(byte);
		return 0;
	}

	return byte; //loopback
}

void CAN::listen()
{
	sleep(1);
}
