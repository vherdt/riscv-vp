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
    Color raw[screenWidth][screenHeight];
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
