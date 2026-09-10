#pragma once

#include <chrono>
#include <cstdint>
#include <functional>

/// Clock and wait service owned by the receiver worker; replay supplies virtual time.
struct GPSDriverClock
{
    std::function<uint64_t()> nowUs;
    std::function<void(std::chrono::microseconds)> wait;
};
