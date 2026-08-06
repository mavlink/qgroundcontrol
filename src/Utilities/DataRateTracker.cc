#include "DataRateTracker.h"

DataRateTracker::DataRateTracker()
{
    _timer.start();
}

void DataRateTracker::recordBytes(qsizetype bytes)
{
    _windowBytes += bytes;
    _totalBytes += static_cast<quint64>(bytes);

    const qint64 elapsed = _timer.elapsed();
    if (elapsed >= kWindowMs) {
        _currentRate = static_cast<double>(_windowBytes) / static_cast<double>(elapsed) * 1000.0;
        _windowBytes = 0;
        _rateUpdated = true;
        (void) _timer.restart();
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
    _timer.restart();
}
