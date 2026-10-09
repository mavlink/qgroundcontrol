#pragma once

#include <chrono>
#include <cstdint>

#include "GPSConnectionErrors.h"
#include "GPSDriver.h"
#include "MonotonicClock.h"

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
    bool update(const GPSReceiveResult& result, bool identified, uint64_t nowUs)
    {
        if (result.liveness != GPSReceiveLiveness::Idle) {
            _bytesAtUs = nowUs;
        }
        if (result.liveness == GPSReceiveLiveness::Data) {
            _dataAtUs = nowUs;
        }
        if (result.updates.testFlag(GPSReceiveUpdate::Position)) {
            _positionAtUs = nowUs;
        }
        const auto recent = [this, nowUs](uint64_t atUs) { return MonotonicClock::fresh(atUs, nowUs, _window); };
        GPSInputProblem current = GPSInputProblem::NotGNSS;
        if (recent(_positionAtUs)) {
            current = GPSInputProblem::None;
        } else if (!recent(_bytesAtUs)) {
            current = GPSInputProblem::NoData;
        } else if (identified) {
            current = GPSInputProblem::NoPositions;
        } else if (recent(_dataAtUs)) {
            current = GPSInputProblem::CorrectionsOnly;
        }
        const bool changed = current != _problem;
        _problem = current;
        return changed;
    }

    [[nodiscard]] GPSInputProblem problem() const { return _problem; }

private:
    std::chrono::milliseconds _window;
    uint64_t _positionAtUs;
    uint64_t _bytesAtUs = 0;
    uint64_t _dataAtUs = 0;
    GPSInputProblem _problem = GPSInputProblem::None;
};
