#pragma once
#include <inttypes.h>
#include <cassert>
#include <cstring>

#define SHMKEY 1338
static constexpr uint16_t screenWidth = 800;
static constexpr uint16_t screenHeight = 600;


typedef uint16_t Color;

struct Frame
{
    Color raw[screenHeight][screenWidth];   //Notice: Screen is on side
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
        memcpy(&getInactiveFrame(), &getActiveFrame(), sizeof(Frame));
    }
    void applyFrameHwAccelerated()
    {
    	activeFrame ++;
    }
};
