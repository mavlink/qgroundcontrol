#pragma once

#include <chrono>

#include "GPSRuntimeIO.h"
#include "GPSTestClock.h"

/// Runtime services on @p clock: waits advance it instead of sleeping and every baud rate is accepted. Tests add the
/// link, or take it from ScriptedReceiver::makeIO().
inline GPSRuntimeIO makeGPSRuntimeTestIO(GPSTestClock& clock)
{
    GPSRuntimeIO io;
    io.nowUs = [&clock] { return clock.nowUs(); };
    io.wait = [&clock](std::chrono::microseconds delay) {
        clock.advanceBy(static_cast<uint64_t>(delay.count()));
        return true;
    };
    io.setBaudrate = [](unsigned) { return GPSBaudStatus::Configured; };
    return io;
}
