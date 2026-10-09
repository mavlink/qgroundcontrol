#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include "GPSClock.h"
#include "GPSCommand.h"
#include "GPSProtocolEvent.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverReports.h"
#include "GPSType.h"

class GPSTransport;
struct GPSConfig;
struct GPSReceiverFamily;
struct GPSRuntimeIO;
struct GPSRuntimeObserver;

/// Sinks the driver pushes decoded data into, invoked on the caller thread from
/// within configure()/receiveOutcome(). Sinks must not call back into the driver.
struct GPSDriverSinks
{
    std::function<void(const GPSPositionReport&)> onPosition;
    std::function<void(const GPSSatelliteReport&)> onSatelliteInfo;
    /// One complete frame; copying it shares the receiver's buffer.
    std::function<void(const QByteArray&)> onRTCM;
    std::function<void(const GPSSurveyReport&)> onSurveyIn;
    /// The family a GPSType::automatic driver detected, before it configures the receiver. A passive driver reports
    /// the protocol its input carries once identified and whenever it changes: the family whose native protocol it
    /// decodes, or GPSType::passive for standard NMEA.
    std::function<void(GPSType)> onReceiverDetected;
};

/// What one receive cycle heard from the receiver.
enum class GPSReceiveLiveness
{
    Idle,
    /// Diagnostics or partial input, never proof of navigation liveness.
    Activity,
    /// Useful reports, even without a registered sink.
    Data,
};

struct [[nodiscard]] GPSReceiveResult
{
    /// The failure that ended the session, or None.
    GPSProtocolError error = GPSProtocolError::None;
    GPSReceiveLiveness liveness = GPSReceiveLiveness::Idle;
    /// Position and satellite reports published to the sinks.
    GPSReceiveUpdates updates = {};
    QString detail = {};

    [[nodiscard]] bool terminal() const
    {
        return error != GPSProtocolError::None && error != GPSProtocolError::Cancelled;
    }
};

/// Hosts a native receiver family and adapts its decoded events to public reports. GPSType::automatic detects the
/// family (GPSReceiverDetector) on every configure(), then fits the request to it: consent to persistent changes lapses
/// where the family has none, a compact-observation request lapses with a warning where the family has no compact
/// option, and an unsupported base mode fails, naming the family.
class GPSDriver
{
public:
    /// Deadlines, report times and satellite freshness run on @a clock; the default is the steady clock.
    GPSDriver(GPSType type, GPSTransport& transport, const GPSReceiverConfig& config, GPSDriverSinks sinks,
              GPSClock clock = {});
    ~GPSDriver();

    Q_DISABLE_COPY(GPSDriver)

    /// Create and configure the underlying driver.
    bool configure();

    /// Failures take precedence over reports in the same cycle; receiving before a successful configure() is
    /// InvalidArgument.
    [[nodiscard]] GPSReceiveResult receiveOutcome(std::chrono::milliseconds timeout);

    /// Latest configure() attempt; remains available after failure. Caller-thread access only.
    [[nodiscard]] const std::vector<GPSConfigurationEvidence>& configurationEvidence() const;

    /// Diagnostic from the latest configure() failure; cleared when a new attempt starts.
    [[nodiscard]] const QString& configurationError() const;

    /// Whether the latest configure() failed because the receiver needs persistent changes the request did not allow.
    [[nodiscard]] bool configurationNeedsConsent() const;

    /// Receiver model and firmware reported by the configured protocol; empty when unknown.
    [[nodiscard]] QString receiverIdentity() const;

private:
    bool _configureDetected(GPSRuntimeIO io, GPSRuntimeObserver observer, unsigned baudrate);
    /// @a detected describes the detection that chose @a family; empty for a configured type.
    bool _configureFamily(const GPSReceiverFamily& family, GPSRuntimeIO io, GPSRuntimeObserver observer,
                          const GPSConfig& config, unsigned baudrate, const QString& detected);
    void _publish(const GPSEventBatch& batch);
    void _handle(const GPSIntegrityReport& report);
    void _handle(const GPSDecodedPosition& report);
    void _handle(const GPSDecodedSatellites& report);
    void _handle(const GPSDecodedSatelliteUsage& report);
    void _handle(const GPSSurveyReport& report);
    void _handle(const GPSRTCMFrame& report);
    void _handle(const GPSInputProtocol& report);
    void _handleSatellites(const GPSSatelliteReport& report);
    void _publishExpiredSatellites();
    void _publishSatellites(const GPSSatelliteReport& report);

    GPSType _type;
    GPSTransport& _transport;
    GPSReceiverConfig _config;
    GPSDriverSinks _sinks;
    GPSClock _clock;

    struct State;
    std::unique_ptr<State> _state;
};
