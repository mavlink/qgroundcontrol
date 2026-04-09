#pragma once

#include <cstdint>

namespace QGC {

/// Edge-triggered consecutive-event counter. record() returns true exactly once when the count
/// crosses `threshold` on the rising edge; subsequent record() calls return false until reset().
template <typename CountT = uint8_t>
class EdgeTriggeredCounter
{
public:
    explicit EdgeTriggeredCounter(CountT threshold) : _threshold(threshold) {}

    /// Returns true on the rising-edge crossing into >= threshold.
    bool record()
    {
        ++_count;
        if (_count >= _threshold && !_armed) {
            _armed = true;
            return true;
        }
        return false;
    }

    void reset()
    {
        _count = 0;
        _armed = false;
    }

    CountT count() const { return _count; }
    bool armed() const { return _armed; }

private:
    const CountT _threshold;
    CountT _count = 0;
    bool _armed = false;
};

}  // namespace QGC
