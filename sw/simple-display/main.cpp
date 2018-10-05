#include "libDisplay.hpp"

#include <iostream>
#include <climits>
#include <cstring>
#include <math.h>

Framebuffer* volatile const framebuffer = (Framebuffer* volatile const)(0x72000000);

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
		display::drawLine(framebuffer->getBackground(), Point(0, i), Point(screenWidth, i), fromRGB(i & 0x7, 0, i >> 3));
		if(i % 5 == 0)
			display::applyFrame();
	}
}

void drawFunnyRects()
{

	static uint32_t m = 0;
	for(uint16_t i = 0; i < 400; i++)
	{
		//drawLine(framebuffer->getInactiveFrame(), getRandomPoint(), getRandomPoint(), getRandomColor());
		display::drawRect(framebuffer->getInactiveFrame(),
				Point(m%screenWidth, m%screenHeight),
				Point((screenWidth-(m+1)%screenWidth), (screenHeight-(m+1)%screenHeight)),
				fromRGB(255-(m%255), (m%255), i%255));
		display::applyFrame();
		m += 17;
	}
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

	display::fillRect(framebuffer->getInactiveFrame(), baseOL, baseUR, bgColor);
	display::applyFrame();
	Point barOL(baseOL.x + 20, baseOL.y + 20);
	Point barUR(baseUR.x - 20, baseUR.y - 20);

	uint16_t expandingAxisDiff = horizontal ? barUR.x - barOL.x : barUR.y - barOL.y;

	for(uint16_t progress = stepsize; progress <= expandingAxisDiff; progress += stepsize)
	{
		display::fillRect(framebuffer->getInactiveFrame(),
				horizontal ? Point(barOL.x + progress - stepsize, barOL.y) : Point(barOL.x, barOL.y + progress - stepsize),
				horizontal ? Point(barOL.x + progress, barUR.y) : Point(barUR.x, barOL.y + progress),
						progressColor);
		display::applyFrame();
	}
	for(int16_t progress = expandingAxisDiff; progress >= 0; progress -= stepsize)
	{
		display::fillRect(framebuffer->getInactiveFrame(),
				horizontal ? Point(barOL.x + progress - stepsize, barOL.y) : Point(barOL.x, barOL.y + progress - stepsize),
				horizontal ? Point(barOL.x + progress, barUR.y) : Point(barUR.x, barOL.y + progress),
						bgColor);
		display::applyFrame();
	}
}

using namespace std;

int main()
{
	//display::fillBackground(fromRGB(10, 100, 0));
	//display::fillInactiveFrame(fromRGB(10, 100, 0));
	//display::applyFrame();

	cout << "Fill background" << endl;
	fillBackground();
	while(true)
	{
		cout << "Draw H-bar" << endl;
		drawFunnyBar(true);
		display::fillInactiveFrame();
		display::applyFrame();
		cout << "Draw V-bar" << endl;
		drawFunnyBar(false);
		display::fillInactiveFrame();
		display::applyFrame();
		cout << "Draw Rects" << endl;
		drawFunnyRects();
		display::fillInactiveFrame();
		display::applyFrame();
	}
	return 0;
}
