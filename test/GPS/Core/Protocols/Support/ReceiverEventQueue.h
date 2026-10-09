#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <utility>

#include <QtCore/QByteArray>

#include "GPSDeadline.h"
#include "Protocols/Support/GPSTestClock.h"
#include "Protocols/Support/ScriptedReceiver.h"

namespace GPSTest {

/// A deterministic receiver clock: boot/navigation events run during reads and waits, not only writes.
class ReceiverEventQueue
{
public:
    explicit ReceiverEventQueue(GPSTestClock& clock)
        : _clock(clock)
    {}

    void schedule(uint64_t delayUs, std::function<void()> event)
    {
        _events.emplace(_clock.nowUs() + delayUs, std::move(event));
    }

    void advanceTo(uint64_t timeUs)
    {
        timeUs = (std::max) (timeUs, _clock.nowUs());
        while (!_events.empty() && _events.begin()->first <= timeUs) {
            auto event = _events.extract(_events.begin());
            // An event made overdue by a direct clock jump runs at the current time.
            _clock.advanceTo(event.key());
            event.mapped()();
        }
        _clock.advanceTo(timeUs);
    }

    void advanceToNext(uint64_t deadlineUs)
    {
        advanceTo(_events.empty() ? deadlineUs : (std::min) (deadlineUs, _events.begin()->first));
    }

    /// One protocol read of a receiver whose events append its @a output: runs the events of the next @a firstStepUs,
    /// then of the read's @a deadline until @a output reaches @a receiver.
    void serveRead(ScriptedReceiver& receiver, std::string& output, GPSDeadline deadline, uint64_t firstStepUs)
    {
        const auto deliver = [&receiver, &output] {
            if (!output.empty()) {
                receiver.queueReply(QByteArray::fromStdString(std::exchange(output, {})));
            }
        };
        advanceTo((std::min) (_clock.nowUs() + firstStepUs, deadline.untilUs));
        deliver();
        while (!receiver.hasQueuedReadData() && _clock.nowUs() < deadline.untilUs) {
            advanceToNext(deadline.untilUs);
            deliver();
        }
    }

private:
    GPSTestClock& _clock;
    std::multimap<uint64_t, std::function<void()>> _events;
};

}  // namespace GPSTest
