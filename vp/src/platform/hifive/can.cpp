#include "can.h"
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

const char* CAN::registerName(uint8_t id){
	switch(id)
	{
	/*..*/
	case MCP_CANCTRL:
		return "MCP_CANCTRL";
	case MCP_RXF0SIDH:
		return "MCP_RXF0SIDH";
	/*..*/
	case MCP_CNF1:
		return "MCP_CNF1";
	case MCP_CNF2:
		return "MCP_CNF2";
	case MCP_CNF3:
		return "MCP_CNF3";
	/*..*/
	case MCP_CANINTF:
		return "MCP_CANINTF";

	case MCP_TXB1CTRL:
		return "MCP_TXB1CTRL";
	case MCP_TXB2CTRL:
		return "MCP_TXB2CTRL";

	default:
		return "UNKNOWN-RegName";
	}
}

const char* CAN::regValueName(uint8_t id){
	switch(id)
	{
	/*..*/
	case MODE_NORMAL:
		return "MODE_NORMAL";
	case MODE_SLEEP:
		return "MODE_SLEEP";

	case MODE_CONFIG:
		return "MODE_CONFIG";

	case MODE_ONESHOT:
		return "MODE_ONESHOT";


	default:
		return "UNKNOWN-RegValue";
	}
}

const char* CAN::spiInstName(uint8_t id)
{
	switch(id)
	{
	/*..*/
	case MCP_WRITE:
		return "MCP_WRITE";
	case MCP_READ:
		return "MCP_READ";
	case MCP_BITMOD:
		return "MCP_BITMOD";
	case MCP_LOAD_TX0:
		return "MCP_LOAD_TX0";
	case MCP_LOAD_TX1:
		return "MCP_LOAD_TX1";
	case MCP_LOAD_TX2:
		return "MCP_LOAD_TX2";
	case MCP_RTS_TX0:
		return "MCP_RTS_TX0";
	case MCP_RTS_TX1:
		return "MCP_RTS_TX1";
	case MCP_RTS_TX2:
		return "MCP_RTS_TX2";
	case MCP_RTS_ALL:
		return "MCP_RTS_ALL";
	case MCP_READ_RX0:
		return "MCP_READ_RX0";
	case MCP_READ_RX1:
		return "MCP_READ_RX1";
	case MCP_READ_STATUS:
		return "MCP_READ_STATUS";
	case MCP_RX_STATUS:
		return "MCP_RX_STATUS";
	case MCP_RESET:
		return "MCP_RESET";

	default:
		return "UNKNOWN-SpiInst";
	}
}

uint8_t CAN::write(uint8_t byte)
{
	uint8_t ret = 0;
	switch(state)
	{
	case State::init:
		command(byte);
		ret = 0;
		break;
	case State::readRegister:
		ret = readRegister(byte);
		break;
	case State::writeRegister:
		ret = writeRegister(byte);
		break;
	case State::bitmod:
		ret = modifyRegister(byte);
		break;
	case State::loadTX0:
	case State::loadTX1:
	case State::loadTX2:
		ret = loadTxBuf(0, byte);
		break;
	case State::sendTX0:
	case State::sendTX1:
	case State::sendTX2:
	case State::sendALL:
		ret = sendTxBuf(0, byte);
		break;
	case State::getStatus:
		ret = 0;
		state = State::init;
		break;
	default:
		std::cerr << "[CAN] in unknown state!" << std::endl;
	}

	return ret;
}

void CAN::command(uint8_t byte)
{
	//std::cout << "[CAN] " << spiInstName(byte);
	switch(byte)
	{
	case MCP_WRITE:
		state = State::writeRegister;
		break;
	case MCP_READ:
		state = State::readRegister;
		break;
	case MCP_BITMOD:
		state = State::bitmod;
		break;
	case MCP_LOAD_TX0:
		state = State::loadTX0;
		break;
	case MCP_LOAD_TX1:
		state = State::loadTX1;
		break;
	case MCP_LOAD_TX2:
		state = State::loadTX2;
		break;
	case MCP_RTS_TX0:
		state = State::sendTX0;
		break;
	case MCP_RTS_TX1:
		state = State::sendTX1;
		break;
	case MCP_RTS_TX2:
		state = State::sendTX2;
		break;
	case MCP_RTS_ALL:
		state = State::sendALL;
		break;
	case MCP_READ_RX0:
		break;
	case MCP_READ_RX1:
		break;
	case MCP_READ_STATUS:
		state = State::getStatus;
		break;
	case MCP_RX_STATUS:
		break;
	case MCP_RESET:
		//do reset stuff
		state = State::init;
		break;
	default:
		std::cerr << std::hex << unsigned(byte) << " is unknown command " << std::endl;
	}
	//std::cout << std::endl;
}

uint8_t CAN::readRegister(uint8_t byte)
{
	static int16_t selectedRegister = -1;
	uint8_t ret = 0;

	if(selectedRegister < 0)
	{
		selectedRegister = byte;
		//std::cout << "\t[CAN] read select Register " << registerName(selectedRegister) << std::endl;
		ret = 0;
	}
	else
	{
		if(byte != 0)
		{	//end "auto increment"
			selectedRegister = -1;
			command(byte);
			return 0;
		}
		//std::cout << "[CAN] Read on Register " << registerName(selectedRegister) << std::endl;
		if(selectedRegister > MCP_RXB1SIDH)
		{
			std::cerr << "Read on Register too high! ";
			ret = 0;
		}
		else
		{
			ret = registers[selectedRegister];
		}
		selectedRegister ++;
	}
	return ret;
}

uint8_t CAN::writeRegister(uint8_t byte)
{
	static int16_t selectedRegister = -1;
	if(selectedRegister < 0)
	{
		selectedRegister = byte;
		//std::cout << "\t[CAN] Write select Register "  << registerName(selectedRegister) << std::endl;
		return 0;
	}
	else
	{
		//std::cout << "[CAN] Write on Register " << registerName(selectedRegister) << std::endl;

		if(selectedRegister > MCP_RXB1SIDH)
		{
			std::cerr << "Write on Register too high! ";
			return 0;
		}
		else
		{
			registers[selectedRegister] = byte;
		}
		selectedRegister = -1;
		state = State::init;
	}
	return 0;
}

uint8_t CAN::modifyRegister(uint8_t byte)
{
	static struct ModReg{
		enum : uint8_t
		{
			_address = 0,
			_mask,
			_data
		} state;
		uint8_t address;
		uint8_t mask;
		uint8_t data;

		ModReg() : state(_address), address(0), mask(0), data(0) {};
	} command;


	//std::cout << "\t[CAN] modReg ";
	switch(command.state)
	{
	case ModReg::_address:
		command.address = byte;
		command.state = ModReg::_mask;
		//std::cout << "select " << registerName(command.address) << std::endl;
		break;
	case ModReg::_mask:
		command.mask = byte;
		command.state  = ModReg::_data;

		//std::cout << "mask " << std::hex << unsigned(byte) << std::endl;
		break;
	case ModReg::_data:
		command.data = byte;
		command.state = ModReg::_address;
		state = State::init;
		//std::cout << "setData " << regValueName(command.data) << std::endl;
		registers[command.address] &= ~command.mask;
		registers[command.address] |= command.data;
		break;
	}
	return 0;
}

uint8_t CAN::loadTxBuf(uint8_t no, uint8_t byte)
{
	static struct LoadTX
	{
		enum
		{
			unknown,
			id,
			length,
			payload,
		} state = unknown;
		uint8_t payload_ptr = 0;
	} command;

	switch(command.state)
	{
	case LoadTX::unknown:
		command.payload_ptr++;
		if(command.payload_ptr == 3)
		{
			command.payload_ptr = 0;
			command.state = LoadTX::id;
		}
		break;
	case LoadTX::id:
		command.state = LoadTX::length;
		txBuf[no].as.id = byte;
		//std::cout << "\t[CAN] ID: " << std::hex << unsigned(byte) << std::endl;
		break;
	case LoadTX::length:
		command.state = LoadTX::payload;
		txBuf[no].as.length = byte;
		//std::cout << "\t[CAN] Length : " << std::hex << unsigned(byte) << std::endl;
		break;
	case LoadTX::payload:
		txBuf[no].as.payload[command.payload_ptr] = byte;
		//std::cout << "\t[CAN] PL : " << std::hex << unsigned(byte) << std::endl;

		command.payload_ptr++;
		if(command.payload_ptr == txBuf[no].as.length){
			new(&command) LoadTX;
			state = State::init;
		}
		break;
	}

	return 0;
}

uint8_t CAN::sendTxBuf(uint8_t no, uint8_t)
{
	std::cout << "\t[CAN] send Data: ";
	for(int i = 0; i < txBuf[no].as.length + 2; i++)
	{
		std::cout << std::hex << unsigned(txBuf[no].raw[i]) << " ";
	}
	std::cout << std::endl;
	registers[MCP_TXB0CTRL] = 0;
	state = State::readRegister;
	return 0;
}

void CAN::listen()
{
	sleep(1);
}
