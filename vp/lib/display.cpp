/*
 * display.cpp
 *
 *  Created on: Sep 11, 2018
 *      Author: dwd
 */

#include "display.hpp"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

Display::Display(sc_module_name)
{
	tsock.register_b_transport(this, &Display::transport);
	createSM();
	memset(frame.raw, 0, sizeof(Framebuffer));
}

void Display::createSM()
{
	int shmid;
	if ((shmid = shmget(SHMKEY, sizeof(Framebuffer), IPC_CREAT | 0666)) < 0)
	{
		std::cerr << "Could not get " << sizeof(Framebuffer) << " Byte of shared Memory " << int(SHMKEY) << " for display" << std::endl;
		perror(NULL);
		exit(1);
	}
	frame.raw = reinterpret_cast<uint8_t*>(shmat(shmid, nullptr, 0));
	if (frame.raw == (uint8_t *) -1) {
		perror("shmat");
		exit(1);
	}
}

void Display::fillFrame(Frame& frame, Color color)
{
	assert(sizeof(Frame) % 8 == 0);
	assert(8 % sizeof(Color) == 0);
	uint64_t* rawFrame = reinterpret_cast<uint64_t*>(frame.raw);
	uint_fast32_t steps = sizeof(Frame) / 8;
	uint64_t wideColor = 0;
	for(uint_fast8_t i = 0; i * sizeof(Color) < 8; i++)
	{
		reinterpret_cast<Color*>(&wideColor)[i] = color;
	}
	for(uint_fast32_t i = 0; i < steps; i++)
	{
		rawFrame[i] = wideColor;
	}
}

void Display::transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay)
{
    tlm::tlm_command cmd = trans.get_command();
    unsigned addr = trans.get_address();
    auto *ptr = trans.get_data_ptr();
    auto len = trans.get_data_length();

    assert ((addr >= 0) && (addr + len <= sizeof(Framebuffer)) && "Access display out of bounds");

	if(cmd == tlm::TLM_WRITE_COMMAND)
	{
		if(addr == offsetof(Framebuffer, command) && len == sizeof(Framebuffer::Command))
		{	//apply command
			switch(*reinterpret_cast<Framebuffer::Command*>(ptr))
			{
			case Framebuffer::Command::fillBackground:
				fillFrame(frame.buf->getBackground(), frame.buf->parameter.fill.color);
				break;
			case Framebuffer::Command::clearAll:
				memset(frame.raw, 0, sizeof(Framebuffer));
				frame.buf->activeFrame++;
				break;
			case Framebuffer::Command::fillInactiveFrame:
				fillFrame(frame.buf->getInactiveFrame(), frame.buf->parameter.fill.color);
				break;
			case Framebuffer::Command::applyFrame:
				frame.buf->activeFrame++;
				memcpy(&frame.buf->getInactiveFrame(), &frame.buf->getActiveFrame(), sizeof(Frame));
				break;
			default:
				cerr << "unknown framebuffer command " << *ptr << endl;
				sc_assert (false);
				break;
			}
			//reset parameter
			memset(&frame.buf->parameter, 0, sizeof(Framebuffer::Parameter));
		}
		else if(addr >= offsetof(Framebuffer, parameter) &&
				addr + len < offsetof(Framebuffer, parameter) + sizeof(Framebuffer::Parameter))
		{	//write the parameter
			memcpy(&frame.raw[addr], ptr, len);
		}
		else
		{	//Write the actual framebuffer
			memcpy(&frame.raw[addr], ptr, len);
		}

	}
	else if (cmd == tlm::TLM_READ_COMMAND && addr < sizeof(Framebuffer))
	{	//read whatever you like
		memcpy(ptr, &frame.raw[addr], len);
	}
	else
	{
		sc_assert (false && "unsupported tlm command");
	}
	delay += sc_core::sc_time(len*5, sc_core::SC_NS);

}
