#pragma once

#include <stdexcept>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

inline void ensure(bool cond) {
    if (!cond)
        throw std::runtime_error("runtime assertion failed");
}

inline void ensure(bool cond, const std::string &reason) {
    if (!cond)
        throw std::runtime_error(reason);
}