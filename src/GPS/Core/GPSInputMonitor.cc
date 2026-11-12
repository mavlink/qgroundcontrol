#include "GPSInputMonitor.h"

#include "GPSDriver.h"
#include "MonotonicClock.h"

bool GPSInputMonitor::update(const GPSReceiveResult& result, bool identified, uint64_t nowUs)
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
