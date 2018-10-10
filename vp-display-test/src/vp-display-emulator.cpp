//============================================================================
// Name        : VP-Display-Emulator.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <thread>
#include <random>
#include <climits>
#include <cstring>

using namespace std;

#define SHMKEY 1338
static constexpr uint16_t screenWidth = 800;
static constexpr uint16_t screenHeight = 600;


typedef uint16_t Color;

struct Frame
{
	Color raw[screenHeight][screenWidth];	//Image is rotated for better conversions!
};

struct __attribute__((packed)) Framebuffer
{
	uint8_t activeFrame;
	Frame frames[2];

	Framebuffer() : activeFrame(0){};

	Frame& getActiveFrame()
	{
		return frames[activeFrame % 2];
	}
	Frame& getInactiveFrame()
	{
		return frames[(activeFrame + 1) % 2];
	}
	void applyFrame()
	{
		activeFrame ++;
		//Copy new frame to next frame
		memcpy(&frames[(activeFrame + 1) % 2], &frames[activeFrame % 2], sizeof(Frame));
	}
};

struct Point
{
	float x;
	float y;
	Point() : x(0), y(0){};
	Point(float x, float y) : x(x), y(y){};
};

uint8_t* createSM()
{
    int shmid;
    uint8_t* sharedMemory;
    if ((shmid = shmget(SHMKEY, sizeof(Framebuffer), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }
    cout << "SHMID: " << shmid << endl;
    sharedMemory = reinterpret_cast<uint8_t*>(shmat(shmid, nullptr, 0));
    if (sharedMemory == (uint8_t *) -1) {
        perror("shmat");
        exit(1);
    }
    return sharedMemory;
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

Point getRandomPoint(std::mt19937& rng)
{
	static std::uniform_int_distribution<std::mt19937::result_type> randX(0,screenWidth-1);
	static std::uniform_int_distribution<std::mt19937::result_type> randY(0,screenHeight-1);
	return Point(randX(rng), randY(rng));
}

Color getRandomColor(std::mt19937& rng)
{
	return std::uniform_int_distribution<std::mt19937::result_type>(0,SHRT_MAX)(rng);
}

int main()
{
	Framebuffer* buffer = new(reinterpret_cast<Framebuffer*>(createSM())) Framebuffer;

	memset(&buffer->getInactiveFrame(), 0, sizeof(Frame));
	buffer->applyFrame();

	std::mt19937 rng;
	rng.seed(std::random_device()());

	while(true)
	{
		drawLine(buffer->getInactiveFrame(), getRandomPoint(rng), getRandomPoint(rng), getRandomColor(rng));
		buffer->applyFrame();
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
