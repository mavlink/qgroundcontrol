#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <utility>

namespace GPSTest {

/// A deterministic receiver clock: boot/navigation events run during reads and waits, not only writes.
class ReceiverEventQueue
{
public:
    explicit ReceiverEventQueue(uint64_t& clock)
        : _clock(clock)
    {}

    void schedule(uint64_t delayUs, std::function<void()> event)
    {
        _events.emplace(_clock + delayUs, std::move(event));
    }

    void advanceTo(uint64_t timeUs)
    {
        timeUs = (std::max) (timeUs, _clock);
        while (!_events.empty() && _events.begin()->first <= timeUs) {
            auto event = _events.extract(_events.begin());
            _clock = (std::max) (_clock, event.key());
            event.mapped()();
        }
        _clock = timeUs;
    }

    void advanceToNext(uint64_t deadlineUs)
    {
        advanceTo(_events.empty() ? deadlineUs : (std::min) (deadlineUs, _events.begin()->first));
    }

private:
    uint64_t& _clock;
    std::multimap<uint64_t, std::function<void()>> _events;
};

}  // namespace GPSTest
