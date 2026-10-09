#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

#include "GPSClock.h"

namespace GPSTest {

/// Virtual microsecond clock shared by reference between the code under test and its receiver models. Reads and
/// advances are atomic, so a receiver worker thread and the test thread can share one clock.
class GPSTestClock
{
public:
    /// A start past zero, which receipt timestamps reserve for "never received".
    static constexpr uint64_t START_US = 1000000;

    explicit GPSTestClock(uint64_t nowUs = 0)
        : _nowUs(nowUs)
    {}

    GPSTestClock(const GPSTestClock&) = delete;
    GPSTestClock& operator=(const GPSTestClock&) = delete;

    uint64_t nowUs() const { return _nowUs.load(); }

    /// Starts a new scenario at @p nowUs; the only operation that may move time backwards.
    void reset(uint64_t nowUs = 0) { _nowUs.store(nowUs); }

    /// Moves time forward to @p timeUs; a time already reached leaves the clock unchanged.
    void advanceTo(uint64_t timeUs)
    {
        uint64_t current = _nowUs.load();
        while (current < timeUs && !_nowUs.compare_exchange_weak(current, timeUs)) {
        }
    }

    void advanceBy(uint64_t durationUs) { _nowUs.fetch_add(durationUs); }

    void advanceBy(std::chrono::microseconds duration) { advanceBy(static_cast<uint64_t>(duration.count())); }

    /// This clock as the clock a GPSDriver or GPSReceiverWorker runs on: waits advance it instead of sleeping.
    GPSClock source()
    {
        return {.nowUs = [this] { return nowUs(); },
                .wait =
                    [this](std::chrono::microseconds duration) {
                        advanceBy(static_cast<uint64_t>(duration.count()));
                        return true;
                    }};
    }

private:
    std::atomic<uint64_t> _nowUs;
};

}  // namespace GPSTest
