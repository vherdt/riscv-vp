#include "../../vp-display/framebuffer.h"
#include <iostream>
#include <climits>
#include <cstring>
#include <math.h>
#include <climits>

static Framebuffer* volatile const framebuffer = (Framebuffer* volatile const)(0x72000000);

using namespace std;

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

Point getRandomPoint()
{
	return Point(rand() % screenWidth, rand() % screenHeight);
}

Color getRandomColor()
{
	return rand()%SHRT_MAX;
}

void fillBackground()
{
	for(uint32_t i = 0; i < screenHeight-1; i ++)
	{
		drawLine(framebuffer->getBackground(), Point(0, i), Point(screenWidth, i), fromRGB(i & 0x7, 0, i >> 3));
		if(i % 5 == 0)
			framebuffer->command = Framebuffer::Command::applyFrame;
	}
}

void drawFunnyRects()
{

	static uint32_t m = 0;
	for(uint16_t i = 0; i < 400; i++)
	{
		//drawLine(framebuffer->getInactiveFrame(), getRandomPoint(), getRandomPoint(), getRandomColor());
		drawRect(framebuffer->getInactiveFrame(),
				Point(m%screenWidth, m%screenHeight),
				Point((screenWidth-(m+1)%screenWidth), (screenHeight-(m+1)%screenHeight)),
				fromRGB(255-(m%255), (m%255), i%255));
		framebuffer->command = Framebuffer::Command::applyFrame;
		m += 17;
	}
	framebuffer->command = Framebuffer::Command::clearInactiveFrame;
}

void drawFunnyBar(bool horizontal = false)
{
	Color bgColor = fromRGB(0, 0, 1);
	Color progressColor = fromRGB(126, 255, 10);
	Point baseOL;
	Point baseUR;
	uint16_t stepsize = 2;
	if(horizontal)
	{
		baseOL = Point (100, 250);
		baseUR = Point (700, 350);
	}
	else
	{
		baseOL = Point (350,  50);
		baseUR = Point (450, 550);
	}

	fillRect(framebuffer->getInactiveFrame(), baseOL, baseUR, bgColor);
	framebuffer->command = Framebuffer::Command::applyFrame;
	Point barOL(baseOL.x + 20, baseOL.y + 20);
	Point barUR(baseUR.x - 20, baseUR.y - 20);

	uint16_t expandingAxisDiff = horizontal ? barUR.x - barOL.x : barUR.y - barOL.y;

	for(uint16_t progress = stepsize; progress <= expandingAxisDiff; progress += stepsize)
	{
		fillRect(framebuffer->getInactiveFrame(),
				horizontal ? Point(barOL.x + progress - stepsize, barOL.y) : Point(barOL.x, barOL.y + progress - stepsize),
				horizontal ? Point(barOL.x + progress, barUR.y) : Point(barUR.x, barOL.y + progress),
						progressColor);
		framebuffer->command = Framebuffer::Command::applyFrame;
	}
	for(int16_t progress = expandingAxisDiff; progress >= 0; progress -= stepsize)
	{
		fillRect(framebuffer->getInactiveFrame(),
				horizontal ? Point(barOL.x + progress - stepsize, barOL.y) : Point(barOL.x, barOL.y + progress - stepsize),
				horizontal ? Point(barOL.x + progress, barUR.y) : Point(barUR.x, barOL.y + progress),
						bgColor);
		framebuffer->command = Framebuffer::Command::applyFrame;
	}
	framebuffer->command = Framebuffer::Command::clearInactiveFrame;
}

int main()
{
	cout << "Fill background" << endl;
	fillBackground();
	while(true)
	{
		cout << "Draw H-bar" << endl;
		drawFunnyBar(true);
		cout << "Draw V-bar" << endl;
		drawFunnyBar(false);
		cout << "Draw Rects" << endl;
		drawFunnyRects();
	}
	return 0;
}
