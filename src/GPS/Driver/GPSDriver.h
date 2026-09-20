#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include <QtCore/QString>

#include "GPSConfigurationEvidence.h"
#include "GPSDriverReports.h"
#include "GPSReceiverConfig.h"
#include "GPSType.h"

class GPSTransport;

/// Sinks the driver pushes decoded data into, invoked on the caller thread from
/// within configure()/receiveOutcome(). Recursive operations are rejected;
/// the caller must keep the driver alive until the enclosing operation returns.
struct GPSDriverSinks
{
    std::function<void(const GPSPositionReport&)> onPosition;
    std::function<void(const GPSSatelliteReport&)> onSatelliteInfo;
    /// Borrowed until the synchronous callback returns.
    std::function<void(std::span<const uint8_t>)> onRTCM;
    std::function<void(const GPSSurveyReport&)> onSurveyIn;
    /// Count-only observations do not imply a list of satellites in view.
    std::function<void(const GPSSatelliteUsageReport&)> onSatelliteUsage;
};

enum class GPSReceiveStatus
{
    Data,
    Activity,
    Idle,
    Cancelled,
    ProtocolError,
    TransportError,
    NotConfigured,
    Busy,
};

struct GPSReceiveResult
{
    GPSReceiveStatus status = GPSReceiveStatus::NotConfigured;
    int updates = 0;
    int errorCode = 0;
    QString detail = {};

    [[nodiscard]] bool terminal() const
    {
        return status == GPSReceiveStatus::ProtocolError || status == GPSReceiveStatus::TransportError ||
               status == GPSReceiveStatus::NotConfigured;
    }
};

/// Selects a native receiver protocol and adapts its decoded events to public reports.
class GPSDriver
{
public:
    GPSDriver(GPSType type, GPSTransport& transport, const GPSReceiverConfig& config, GPSDriverSinks sinks);
    ~GPSDriver();

    GPSDriver(const GPSDriver&) = delete;
    GPSDriver& operator=(const GPSDriver&) = delete;

    /// Whether this build contains the requested native protocol implementation.
    static bool supportsType(GPSType type);

    /// Create and configure the underlying driver. Reentrant calls fail without replacing the active driver.
    bool configure();

    /// Useful reports are Data even without a registered sink. Diagnostics/partial input are Activity,
    /// never proof of navigation liveness. Terminal failures take precedence over reports in the same cycle.
    /// Reentrant calls return Busy without touching the transport or active decoder.
    [[nodiscard]] GPSReceiveResult receiveOutcome(unsigned timeoutMs);

    /// Latest non-reentrant configure() attempt; remains available after failure. Caller-thread access only.
    [[nodiscard]] const std::vector<GPSConfigurationEvidence>& configurationEvidence() const;

    /// Diagnostic from the latest configure() failure; cleared when a new attempt starts.
    [[nodiscard]] const QString& configurationError() const;

private:
    void _publishExpiredSatellites();

    GPSType _type;
    GPSTransport& _transport;
    GPSReceiverConfig _config;
    GPSDriverSinks _sinks;
    bool _operationInProgress = false;

    struct State;
    std::unique_ptr<State> _state;
};
