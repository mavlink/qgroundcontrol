#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include "GPSConfigurationEvidence.h"
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

/// Selects a native receiver protocol and adapts its decoded events to public reports.
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
    /// arrives. Returns a bitset (<0 error, bit0 position, bit1 satellite),
    /// or <0 if not configured.
    int receive(unsigned timeoutMs);

    /// Latest configure() attempt; remains available after failure. Caller-thread access only.
    [[nodiscard]] const std::vector<GPSConfigurationEvidence>& configurationEvidence() const;

private:
    GPSType _type;
    GPSTransport& _transport;
    GPSReceiverConfig _config;
    GPSDriverSinks _sinks;

    struct State;
    std::unique_ptr<State> _state;
};
