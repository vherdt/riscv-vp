#include "../../vp-display/framebuffer.h"
#include <iostream>
#include <climits>
#include <cstring>
#include <math.h>
#include <climits>

static Framebuffer* volatile const framebuffer = (Framebuffer* volatile const)(0x72000000);

struct Point
{
	float x;
	float y;
	Point() : x(0), y(0){};
	Point(float x, float y) : x(x), y(y){};
};

Color fromRGB(uint8_t r, uint8_t g, uint8_t b)
{
	Color ret = 0;
	ret |= (b & 0x0F);
	ret |= (g & 0x0F) << 4;
	ret |= (r & 0x0F) << 8;
	return ret;
}

void drawLine(Frame& frame, Point from, Point to, Color color)
{
    // Bresenham's line algorithm
	const bool steep = (fabs(to.y - from.y) > fabs(to.x - from.x));
	if(steep)
	{
		std::swap(from.x, from.y);
		std::swap(to.x, to.y);
	}

	if(from.x > to.x)
	{
		std::swap(from.x, to.x);
		std::swap(from.y, to.y);
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
		std::swap(ol.x, ur.x);
	}
	if(ol.y > ur.y)
	{
		std::swap(ol.y, ur.y);
	}
	drawLine(frame, ol, Point(ur.x, ol.y), color);
	drawLine(frame, Point(ur.x, ol.y), ur, color);
	drawLine(frame, ur, Point(ol.x, ur.y), color);
	drawLine(frame, Point(ol.x, ur.y), ol, color);
}

Point getRandomPoint()
{
	return Point(rand() % screenWidth, rand() % screenHeight);
}

Color getRandomColor()
{
	return rand()%SHRT_MAX;
}

int main()
{
	for(uint32_t i = 0; i < screenHeight; i ++)
	{
		drawLine(framebuffer->getBackground(), Point(0, i), Point(screenWidth, i), fromRGB(i & 0x7, 0, i >> 3));
		if(i % 3 == 0)
			framebuffer->command = Framebuffer::Command::applyFrame;
	}
	uint32_t m = 0;
	while(true)
	{
		for(uint16_t i = 0; i < 100; i++)
		{
			//drawLine(framebuffer->getInactiveFrame(), getRandomPoint(), getRandomPoint(), getRandomColor());
			drawRect(framebuffer->getInactiveFrame(),
					Point(m%screenWidth, m%screenHeight),
					Point((screenWidth-(m+1)%screenWidth), (screenHeight-(m+1)%screenHeight)),
					fromRGB(255-(m%255), (m%255), i%255));
			framebuffer->command = Framebuffer::Command::applyFrame;
			m += 17;
		}
		framebuffer->command = Framebuffer::Command::clearForeground;
	}
	return 0;
}
