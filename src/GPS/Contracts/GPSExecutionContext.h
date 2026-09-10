#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <thread>

/// Shared worker clock domain. Replay supplies all three time services and cancellation.
struct GPSExecutionContext
{
    std::function<uint64_t()> nowUs = [] {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
    };
    std::function<uint64_t()> utcNowUs = [] {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    };
    std::function<void(std::chrono::microseconds)> wait = [](auto duration) { std::this_thread::sleep_for(duration); };
    std::function<bool()> cancelled = [] { return false; };

    bool waitFor(std::chrono::microseconds duration) const
    {
        while (duration.count() > 0 && !cancelled()) {
            const auto slice = std::min(duration, std::chrono::microseconds(50000));
            wait(slice);
            duration -= slice;
        }
        return !cancelled();
    }
};
