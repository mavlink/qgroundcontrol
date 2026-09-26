#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include "GPSConfigurationEvidence.h"
#include "GPSReceiveUpdates.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverReports.h"
#include "GPSType.h"

class GPSTransport;
struct GPSConfig;
struct GPSEventBatch;
struct GPSReceiverFamily;
struct GPSRuntimeIO;
struct GPSRuntimeObserver;

/// Sinks the driver pushes decoded data into, invoked on the caller thread from
/// within configure()/receiveOutcome(). Recursive operations are rejected;
/// the caller must keep the driver alive until the enclosing operation returns.
struct GPSDriverSinks
{
    std::function<void(const GPSPositionReport&)> onPosition;
    std::function<void(const GPSSatelliteReport&)> onSatelliteInfo;
    /// One complete frame; copying it shares the receiver's buffer.
    std::function<void(const QByteArray&)> onRTCM;
    std::function<void(const GPSSurveyReport&)> onSurveyIn;
    /// The family a GPSType::automatic driver detected, before it configures the receiver.
    std::function<void(GPSType)> onReceiverDetected;
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

struct [[nodiscard]] GPSReceiveResult
{
    GPSReceiveStatus status = GPSReceiveStatus::NotConfigured;
    /// Position and satellite reports published to the sinks.
    GPSReceiveUpdates updates = {};
    QString detail = {};

    [[nodiscard]] bool terminal() const
    {
        return status == GPSReceiveStatus::ProtocolError || status == GPSReceiveStatus::TransportError ||
               status == GPSReceiveStatus::NotConfigured;
    }
};

/// Hosts a native receiver family and adapts its decoded events to public reports. GPSType::automatic detects the
/// family (GPSReceiverDetector) on every configure(), then fits the request to it: consent to persistent changes lapses
/// where the family has none, compact observations fall back to MSM7 with a warning, and an unsupported base mode
/// fails, naming the family.
class GPSDriver
{
public:
    GPSDriver(GPSType type, GPSTransport& transport, const GPSReceiverConfig& config, GPSDriverSinks sinks);
    ~GPSDriver();

    GPSDriver(const GPSDriver&) = delete;
    GPSDriver& operator=(const GPSDriver&) = delete;

    /// Whether a native protocol exists for @a type.
    [[nodiscard]] static bool supportsType(GPSType type);

    /// Create and configure the underlying driver. Reentrant calls fail without replacing the active driver.
    bool configure();

    /// Useful reports are Data even without a registered sink. Diagnostics/partial input are Activity,
    /// never proof of navigation liveness. Terminal failures take precedence over reports in the same cycle.
    /// Reentrant calls return Busy without touching the transport or active decoder.
    [[nodiscard]] GPSReceiveResult receiveOutcome(std::chrono::milliseconds timeout);

    /// Latest non-reentrant configure() attempt; remains available after failure. Caller-thread access only.
    [[nodiscard]] const std::vector<GPSConfigurationEvidence>& configurationEvidence() const;

    /// Diagnostic from the latest configure() failure; cleared when a new attempt starts.
    [[nodiscard]] const QString& configurationError() const;

    /// Whether the latest configure() failed because the receiver needs persistent changes the request did not allow.
    [[nodiscard]] bool configurationNeedsConsent() const;

    /// Receiver model and firmware reported by the configured protocol; empty when unknown.
    [[nodiscard]] QString receiverIdentity() const;

    /// The family the latest configure() of a GPSType::automatic driver detected, also after a later failure.
    [[nodiscard]] std::optional<GPSType> detectedType() const;

private:
    bool _configureDetected(GPSRuntimeIO io, GPSRuntimeObserver observer, unsigned baudrate);
    /// @a detected describes the detection that chose @a family; empty for a configured type.
    bool _configureFamily(const GPSReceiverFamily& family, GPSRuntimeIO io, GPSRuntimeObserver observer,
                          const GPSConfig& config, unsigned baudrate, const QString& detected);
    void _publish(const GPSEventBatch& batch);
    void _publishExpiredSatellites();
    void _publishSatellites(const GPSSatelliteReport& report);

    GPSType _type;
    GPSTransport& _transport;
    GPSReceiverConfig _config;
    GPSDriverSinks _sinks;
    bool _operationInProgress = false;

    struct State;
    std::unique_ptr<State> _state;
};
