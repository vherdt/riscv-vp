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

Point getRandomPoint()
{
	return Point(rand() % screenWidth, rand() % screenHeight);
}

Color getRandomColor()
{
	return rand()%SHRT_MAX;
}

int main() {


	while(true)
	{
		std::cout << "DrawLine" << std::endl;
		drawLine(framebuffer->getInactiveFrame(), getRandomPoint(), getRandomPoint(), getRandomColor());
		std::cout << "applyFrame" << std::endl;
		framebuffer->applyFrameHwAccelerated();
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return 0;
}
