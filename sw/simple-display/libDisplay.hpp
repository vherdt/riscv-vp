/*
 * libDisplay.hpp
 *
 *  Created on: Oct 5, 2018
 *      Author: dwd
 */

#pragma once

#include "../../vp-display/framebuffer.h"

extern Framebuffer* volatile const framebuffer;


inline Color fromRGB(uint8_t r, uint8_t g, uint8_t b)
{
	Color ret = 0;
	ret |= (b & 0x0F);
	ret |= (g & 0x0F) << 4;
	ret |= (r & 0x0F) << 8;
	return ret;
}

namespace display
{
	void drawLine(Frame& frame, Point from, Point to, Color color);

	void drawRect(Frame& frame, Point ol, Point ur, Color color);

	void fillRect(Frame& frame, Point ol, Point ur, Color color);

	void applyFrame();

	void fillInactiveFrame(Color color = 0);

	void fillBackground(Color color = 0);
};
