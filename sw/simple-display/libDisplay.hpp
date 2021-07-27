/*
 * libDisplay.hpp
 *
 *  Created on: Oct 5, 2018
 *      Author: dwd
 */

#pragma once

#include "../../env/basic/vp-display/framebuffer.h"

extern Framebuffer* volatile const framebuffer;

inline Framebuffer::Color fromRGB(uint8_t r, uint8_t g, uint8_t b) {
	Framebuffer::Color ret = 0;
	ret |= (b & 0x0F);
	ret |= (g & 0x0F) << 4;
	ret |= (r & 0x0F) << 8;
	return ret;
}

namespace display {
void setPixel(Framebuffer::Type frame, Framebuffer::Point pixel, Framebuffer::Color color);

void drawLine(Framebuffer::Type frame, Framebuffer::PointF from, Framebuffer::PointF to, Framebuffer::Color color);

void drawRect(Framebuffer::Type frame, Framebuffer::PointF ol, Framebuffer::PointF ur, Framebuffer::Color color);

void fillRect(Framebuffer::Type frame, Framebuffer::PointF ol, Framebuffer::PointF ur, Framebuffer::Color color);

void applyFrame();

void fillFrame(Framebuffer::Type frame = Framebuffer::Type::foreground, Framebuffer::Color color = 0);
};  // namespace display
