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
	if ((shmid = shmget(SHMKEY, sizeof(Framebuffer), IPC_CREAT | 0666)) < 0) {
		perror("shmget");
		exit(1);
	}
	frame.raw = reinterpret_cast<uint8_t*>(shmat(shmid, nullptr, 0));
	if (frame.raw == (uint8_t *) -1) {
		perror("shmat");
		exit(1);
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
		if(addr == offsetof(Framebuffer, command))
		{
			assert(len == sizeof(Framebuffer::Command));
			switch(*reinterpret_cast<Framebuffer::Command*>(ptr))
			{
			case Framebuffer::Command::clearBackground:
				memset(&frame.buf->getBackground(), 0, sizeof(Frame));
				break;
			case Framebuffer::Command::clearAll:
				memset(frame.raw, 0, sizeof(Framebuffer));
				frame.buf->activeFrame++;
				break;
			case Framebuffer::Command::clearInactiveFrame:
				memset(&frame.buf->getInactiveFrame(), 0, sizeof(Frame));
				break;
			case Framebuffer::Command::applyFrame:
				frame.buf->activeFrame++;
				memcpy(&frame.buf->getInactiveFrame(), &frame.buf->getActiveFrame(), sizeof(Frame));
				break;
			default:
				break;
			}
		}
		else
		{
			memcpy(&frame.raw[addr], ptr, len);
		}

	}
	else if (cmd == tlm::TLM_READ_COMMAND)
	{
		memcpy(ptr, &frame.raw[addr], len);
	}
	else
	{
		sc_assert (false && "unsupported tlm command");
	}
	delay += sc_core::sc_time(len*5, sc_core::SC_NS);

}
