#pragma once

#include <algorithm>
#include <cstdint>

/// Virtual microsecond clock shared by reference between a protocol test IO and its receiver models.
class GPSTestClock
{
public:
    explicit GPSTestClock(uint64_t nowUs = 0)
        : _nowUs(nowUs)
    {}

    uint64_t nowUs() const { return _nowUs; }

    /// Starts a new scenario at @p nowUs; the only operation that may move time backwards.
    void reset(uint64_t nowUs = 0) { _nowUs = nowUs; }

    /// Moves time forward to @p timeUs; a time already reached leaves the clock unchanged.
    void advanceTo(uint64_t timeUs) { _nowUs = (std::max) (_nowUs, timeUs); }

    void advanceBy(uint64_t durationUs) { _nowUs += durationUs; }

private:
    uint64_t _nowUs;
};
