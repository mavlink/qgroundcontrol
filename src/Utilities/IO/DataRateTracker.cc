#include "DataRateTracker.h"

#include <utility>

DataRateTracker::DataRateTracker(Clock clock)
    : _clock(clock ? std::move(clock) : Clock{MonotonicClock::nowUs})
{
    _windowStartedUs = _clock();
}

void DataRateTracker::recordBytes(qsizetype bytes)
{
    _windowBytes += bytes;
    _totalBytes += static_cast<quint64>(bytes);

    refresh();
}

void DataRateTracker::refresh()
{
    const quint64 now = _clock();
    const quint64 elapsed = now >= _windowStartedUs ? now - _windowStartedUs : 0;
    if (elapsed >= kWindowUs) {
        _currentRate = static_cast<double>(_windowBytes) / static_cast<double>(elapsed) * 1000000.0;
        _windowBytes = 0;
        _rateUpdated = true;
        _windowStartedUs = now;
    } else {
        _rateUpdated = false;
    }
}

void DataRateTracker::reset()
{
    _totalBytes = 0;
    _windowBytes = 0;
    _currentRate = 0.0;
    _rateUpdated = false;
    _windowStartedUs = _clock();
}
