#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>

#include "GPSDriver.h"
#include "GPSReceiverConfig.h"
#include "GPSType.h"

class GPSTransport;

/// Test-only baseline of f1e7700e0, using the production facade's config and sinks.
/// Facade over the px4-gpsdrivers library: selects and configures the receiver
/// driver, bridges its callbacks to a GPSTransport plus the supplied sinks, and
/// pumps its receive loop. Keeps all px4 headers and types out of callers.
class GPSLegacyDriver
{
public:
    GPSLegacyDriver(GPSType type, GPSTransport& transport, const GPSReceiverConfig& config, GPSDriverSinks sinks);
    ~GPSLegacyDriver();

    GPSLegacyDriver(const GPSLegacyDriver&) = delete;
    GPSLegacyDriver& operator=(const GPSLegacyDriver&) = delete;

    /// Create and configure the underlying driver. Returns false on failure.
    bool configure();

    /// Pump one receive cycle, invoking the position/satellite sinks as data
    /// arrives. Returns the px4 bitset (<0 error, bit0 position, bit1 satellite),
    /// or <0 if not configured.
    int receive(unsigned timeoutMs);

    /// Trampoline target for the px4 callback; `type` is a GPSCallbackType value.
    /// Public only so the file-local C callback can reach it — not for callers.
    int handleCallback(int type, void* data1, int data2);

private:
    GPSType _type;
    GPSTransport& _transport;
    GPSReceiverConfig _config;
    GPSDriverSinks _sinks;

    struct State;
    std::unique_ptr<State> _state;
};
