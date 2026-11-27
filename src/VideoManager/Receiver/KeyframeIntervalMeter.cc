#include "KeyframeIntervalMeter.h"

#include <algorithm>

void KeyframeIntervalMeter::note(std::chrono::nanoseconds time)
{
    if (_count > 0) {
        const std::chrono::nanoseconds previous = _times[_count - 1];
        if (time == previous) {
            return;
        }
        if (time < previous) {
            _count = 0;
        }
    }
    if (_count == kWindow) {
        std::shift_left(_times.begin(), _times.end(), 1);
        --_count;
    }
    _times[_count++] = time;
    if (_count >= 2) {
        const std::chrono::nanoseconds span = _times[_count - 1] - _times[0];
        _intervalNs.store(span.count() / static_cast<qint64>(_count - 1), std::memory_order_relaxed);
    }
}

double KeyframeIntervalMeter::intervalMs() const
{
    const qint64 intervalNs = _intervalNs.load(std::memory_order_relaxed);
    return (intervalNs < 0) ? -1.0 : (static_cast<double>(intervalNs) / 1e6);
}
