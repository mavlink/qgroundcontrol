#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>

#include "GPSDriverReports.h"
#include "GPSReceiverConfig.h"
#include "GPSType.h"

class GPSTransport;

/// Sinks the driver pushes decoded data into, invoked on the caller thread from
/// within configure()/receive().
struct GPSDriverSinks
{
    std::function<void(const GPSPositionReport&)> onPosition;
    std::function<void(const GPSSatelliteReport&)> onSatelliteInfo;
    /// Borrowed until the synchronous callback returns.
    std::function<void(std::span<const uint8_t>)> onRTCM;
    std::function<void(const GPSSurveyReport&)> onSurveyIn;
};

/// Facade over the px4-gpsdrivers library: selects and configures the receiver
/// driver, bridges its callbacks to a GPSTransport plus the supplied sinks, and
/// pumps its receive loop. Keeps all px4 headers and types out of callers.
class GPSDriver
{
public:
    GPSDriver(GPSType type, GPSTransport& transport, const GPSReceiverConfig& config, GPSDriverSinks sinks);
    ~GPSDriver();

    GPSDriver(const GPSDriver&) = delete;
    GPSDriver& operator=(const GPSDriver&) = delete;

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
