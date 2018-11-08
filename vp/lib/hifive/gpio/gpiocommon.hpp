/*
 * gpio.hpp
 *
 *  Created on: 7 Nov 2018
 *      Author: dwd
 */

#pragma once

#include <inttypes.h>
#include <stddef.h>

void hexPrint(unsigned char* buf, size_t size);
void bitPrint(unsigned char* buf, size_t size);


struct GpioCommon
{
	typedef uint64_t Reg;

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

	Reg state;
	void printRequest(Request* req);
	GpioCommon();
};
