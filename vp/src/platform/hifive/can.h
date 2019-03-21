#pragma once

#include "spi.h"
#include <functional>
#include <thread>

class CAN : public SpiInterface
{
	enum class State
	{
		init,
		readRegister,

		shit,
		wank,
		fuck,
		arse,
		crap,
		dick,
	} state ;

	std::thread listener;

public:
	CAN();
	~CAN();

	uint8_t write(uint8_t byte) override;

	void command(uint8_t byte);

	void listen();
};
