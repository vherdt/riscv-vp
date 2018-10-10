#pragma once
#include "framebuffer.h"
#include <functional>
#include <thread>
#include <atomic>

class VPDisplayserver
{
    unsigned int mSharedMemoryKey;
    Framebuffer* framebuffer;
    std::atomic<bool> stop;
    std::thread active_watch;
public:
    VPDisplayserver(unsigned int sharedMemoryKey = 1338);
    ~VPDisplayserver();
    Framebuffer* createSM();
    void startListening(std::function<void(bool)> notifyChange);
};
