#include "GPSPositionSourceSelector.h"

#include <algorithm>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSPositionSourceSelectorLog, "GPS.PositionManager.GPSPositionSourceSelector")

GPSPositionSourceSelector::GPSPositionSourceSelector()
{
    qCDebug(GPSPositionSourceSelectorLog) << this;
}

GPSPositionSourceSelector::~GPSPositionSourceSelector()
{
    qCDebug(GPSPositionSourceSelectorLog) << this;
}

int GPSPositionSourceSelector::select(const std::array<Candidate, 3>& priority, int current, qint64 nowMs,
                                      int recoveryDelayMs)
{
    std::optional<int> best;
    bool currentAvailable = false;
    bool currentHealthy = false;
    for (const auto& source : priority) {
        if (source.available && source.id == current) {
            currentAvailable = true;
            currentHealthy = source.healthy;
        }
        if (!best && source.available && source.healthy) {
            best = source.id;
        }
    }
    if (best && *best != current && currentHealthy) {
        if (_candidate != best) {
            _candidate = best;
            _candidateSinceMs = nowMs;
        }
        if (remainingRecoveryMs(nowMs, recoveryDelayMs) > 0) {
            return current;
        }
    }
    reset();
    if (best) {
        return *best;
    }
    if (currentAvailable) {
        return current;
    }
    for (const auto& source : priority) {
        if (source.available) {
            return source.id;
        }
    }
    return priority.back().id;
}

void GPSPositionSourceSelector::reset()
{
    _candidate.reset();
}

int GPSPositionSourceSelector::remainingRecoveryMs(qint64 nowMs, int recoveryDelayMs) const
{
    return _candidate ? static_cast<int>((std::max) (qint64(0), recoveryDelayMs - (nowMs - _candidateSinceMs))) : 0;
}
