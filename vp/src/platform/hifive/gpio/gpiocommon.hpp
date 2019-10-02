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

struct GpioCommon {
	static constexpr unsigned default_port = 1400;
	typedef uint64_t Reg;
	typedef uint8_t Tristate;

	enum Operation : uint8_t { GET_BANK = 1, SET_BIT, SET_PWM };

	struct Request {
		Operation op;
		union {
			struct {
				uint8_t pos : 6;
				Tristate val : 2;
			} setBit;
			/*
			struct
			{
			        uint8_t pos;
			        uint8_t percent;
			} setPWM;
			*/
		};
	};

	Reg state;
	void printRequest(Request* req);
	GpioCommon();
};
