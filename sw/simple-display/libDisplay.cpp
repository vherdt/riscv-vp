/*
 * libDisplay.cpp
 *
 *  Created on: Oct 5, 2018
 *      Author: dwd
 */

#include "libDisplay.hpp"
#include <algorithm>
#include <math.h>

using namespace std;

namespace display
{
	void drawLine(Frame& frame, Point from, Point to, Color color)
	{
		if(from.x == to.x)
		{	//vertical line
			if(from.y > to.y)
				swap(from.y, to.y);
			uint16_t intFromX = from.x;
			uint16_t intToY = to.y;
			for(uint16_t y = from.y; y <= intToY; y++)
			{
				frame.raw[y][intFromX] = color;
			}
			return;
		}
		if(from.y == to.y)
		{	//horizontal line, the fastest
			if(from.x > to.x)
				swap(from.x, to.x);
			uint16_t intFromY = from.y;
			uint16_t intToX = to.x;
			for(uint16_t x = from.x; x <= intToX; x++)
			{
				frame.raw[intFromY][x] = color;
			}
			return;
		}

		// Bresenham's line algorithm
		const bool steep = (fabs(to.y - from.y) > fabs(to.x - from.x));
		if(steep)
		{
			swap(from.x, from.y);
			swap(to.x, to.y);
		}

		if(from.x > to.x)
		{
			swap(from.x, to.x);
			swap(from.y, to.y);
		}

		const float dx = to.x - from.x;
		const float dy = fabs(to.y - from.y);

		float error = dx / 2.0f;
		const int ystep = (from.y < to.y) ? 1 : -1;
		int y = (int)from.y;

		const int maxX = (int)to.x;

		for(int x = (int)from.x; x < maxX; x++)
		{
			if(steep)
			{
				frame.raw[x][y] = color;
			}
			else
			{
				frame.raw[y][x] = color;
			}

			error -= dy;
			if(error < 0)
			{
				y += ystep;
				error += dx;
			}
		}
	}

	void drawRect(Frame& frame, Point ol, Point ur, Color color)
	{
		if(ol.x > ur.x)
		{
			swap(ol.x, ur.x);
		}
		if(ol.y > ur.y)
		{
			swap(ol.y, ur.y);
		}
		drawLine(frame, ol, Point(ur.x, ol.y), color);
		drawLine(frame, Point(ur.x, ol.y), ur, color);
		drawLine(frame, ur, Point(ol.x, ur.y), color);
		drawLine(frame, Point(ol.x, ur.y), ol, color);
	}

	void fillRect(Frame& frame, Point ol, Point ur, Color color)
	{
		if(ol.x > ur.x)
		{
			swap(ol.x, ur.x);
		}
		if(ol.y > ur.y)
		{
			swap(ol.y, ur.y);
		}
		if(ur.x - ol.x > ur.y - ol.y)
		{
			//Horizontal
			for(uint16_t y = ol.y; y <= ur.y; y++)
			{
				drawLine(frame, Point(ol.x, y), Point(ur.x, y), color);
			}
		}
		else
		{
			//Vertical
			for(uint16_t x = ol.x; x <= ur.x; x++)
			{
				drawLine(frame, Point(x, ol.y), Point(x, ur.y), color);
			}
		}
	}

	void applyFrame()
	{
		framebuffer->command = Framebuffer::Command::applyFrame;
	}

	void fillInactiveFrame(Color color)
	{
		framebuffer->parameter.fill.color = color;
		framebuffer->command = Framebuffer::Command::fillInactiveFrame;
	}

	void fillBackground(Color color)
	{
		framebuffer->parameter.fill.color = color;
		framebuffer->command = Framebuffer::Command::fillBackground;
	}

}
