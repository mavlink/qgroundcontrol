#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

/// Host-timed survey-in duration for receivers that report completion but not elapsed time.
class GPSSurveyClock
{
public:
    void start(uint64_t nowUs)
    {
        _startedUs = nowUs;
        _seconds = 0;
    }

    void reset()
    {
        _startedUs.reset();
        _seconds = 0;
    }

    /// Freezes the duration at its final value.
    void stop(uint64_t nowUs)
    {
        (void) update(nowUs);
        _startedUs.reset();
    }

    bool running() const { return _startedUs.has_value(); }

    /// @return true when the whole-second duration advanced.
    bool update(uint64_t nowUs)
    {
        if (!_startedUs || nowUs < *_startedUs) {
            return false;
        }
        const auto seconds = static_cast<uint32_t>((nowUs - *_startedUs) / 1000000);
        if (seconds == _seconds) {
            return false;
        }
        _seconds = seconds;
        return true;
    }

    std::chrono::seconds duration() const { return std::chrono::seconds(_seconds); }

private:
    std::optional<uint64_t> _startedUs;
    uint32_t _seconds = 0;
};
