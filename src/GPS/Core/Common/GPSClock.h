#pragma once

#include <chrono>
#include <cstdint>
#include <functional>

#include "MonotonicClock.h"

/// The monotonic clock receiver sessions run on: the steady clock unless tests inject a virtual one.
struct GPSClock
{
    std::function<uint64_t()> nowUs = MonotonicClock::nowUs;
    /// Waits @a duration on this clock; false when the wait was cancelled. Empty means a real wait.
    std::function<bool(std::chrono::microseconds)> wait{};
};
