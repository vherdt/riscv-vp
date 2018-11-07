/*
 * gpio.hpp
 *
 *  Created on: 7 Nov 2018
 *      Author: dwd
 */

#pragma once

#include <inttypes.h>
#include <stddef.h>

void hexPrint(char* buf, size_t size);
void bitPrint(char* buf, size_t size);


struct Gpio
{
	typedef uint64_t Reg;
	struct State
	{
		Reg val;
		uint16_t interrupt_route[64];
	};

	enum Operation : uint8_t
	{
		GET_BANK = 1,
		SET_BIT
	};

	struct Request
	{
		Operation op;
		union
		{
			struct
			{
				uint8_t pos : 6;
				uint8_t tristate : 2;
			} setBit;
		};
	};
	State state;
	void printRequest(Request* req);
	Gpio();
};
