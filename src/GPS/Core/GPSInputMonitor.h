#pragma once

#include <chrono>
#include <cstdint>

#include "GPSConnectionErrors.h"

struct GPSReceiveResult;

/// Classifies why a session that stays connected delivers no position, from the receive cycles of the last @a window.
class GPSInputMonitor
{
public:
    GPSInputMonitor(uint64_t startUs, std::chrono::milliseconds window)
        : _window(window)
        , _positionAtUs(startUs)
    {}

    /// Records one receive cycle at @a nowUs; @a identified is whether the input's protocol was identified.
    /// @return whether problem() changed.
    bool update(const GPSReceiveResult& result, bool identified, uint64_t nowUs);

    [[nodiscard]] GPSInputProblem problem() const { return _problem; }

private:
    std::chrono::milliseconds _window;
    uint64_t _positionAtUs;
    uint64_t _bytesAtUs = 0;
    uint64_t _dataAtUs = 0;
    GPSInputProblem _problem = GPSInputProblem::None;
};
