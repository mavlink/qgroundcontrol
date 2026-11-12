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
#include "GPSDetectionHint.h"
#include "GPSProtocolEvent.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverReports.h"
#include "GPSType.h"

class GPSProtocolRuntime;
class GPSTransport;
struct GPSConfig;
struct GPSReceiverDetection;
struct GPSReceiverFamily;
struct GPSRuntimeIO;
struct GPSRuntimeObserver;

/// Sinks the driver pushes decoded data into, invoked on the caller thread from
/// within configure()/receiveOutcome(). Sinks must not call back into the driver.
struct GPSDriverSinks
{
    std::function<void(const GPSPositionReport&)> onPosition;
    std::function<void(const GPSSatelliteReport&)> onSatelliteInfo;
    /// One complete frame; copying it shares the receiver's buffer. A base configured here delivers frames only while
    /// its base status is Valid; passive input and Septentrio receivers deliver every frame.
    std::function<void(const QByteArray&)> onRTCM;
    /// Also delivered while configure() runs.
    std::function<void(const GPSBaseStatus&)> onBaseStatus;
    /// The family a GPSType::Automatic driver detected, before it configures the receiver. A passive driver reports
    /// the protocol its input carries once identified and whenever it changes: the family whose native protocol it
    /// decodes, or GPSType::Passive for standard NMEA. A change first withdraws the satellite counts of the protocol
    /// left behind, with an empty onSatelliteInfo report.
    std::function<void(GPSType)> onReceiverDetected;
};

/// Observers of what a driver's detection and runtime produce, before the driver adapts and filters it for the sinks:
/// for the progress a session shows while it detects, and for tests and tools that record a session. Invoked on the
/// caller thread from within configure()/receiveOutcome(); they must not call back into the driver.
struct GPSDriverObserver
{
    /// Every completed command: detection, configuration and streaming alike.
    std::function<void(const GPSConfigurationEvidence&)> commandFinished;
    /// Every decoded batch, before the sinks receive what the driver publishes of it.
    std::function<void(const GPSEventBatch&)> decoded;
    /// The detection a GPSType::Automatic driver ran from @a baud, whether or not it identified a family.
    std::function<void(const GPSReceiverDetection&, unsigned baud)> detected;
    /// The runtime after it configured its family from @a requestedBaud and ended at @a baud, before a failed one is
    /// discarded. After a success it stays the driver's until the next configure(), and input decoded through it
    /// (GPSProtocolRuntime::consume()) reaches the sinks as received input does.
    std::function<void(GPSProtocolRuntime&, unsigned requestedBaud, unsigned baud)> configured;
    /// A GPSType::Automatic driver now looks for the receiver with the link at @a baud: for @a family alone, where
    /// its hint says the receiver was, or for any family (GPSType::Automatic) as receiver detection scans that rate.
    std::function<void(GPSType family, unsigned baud)> detecting;
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

/// Hosts a native receiver family and adapts its decoded events to public reports. GPSType::Automatic detects the
/// family on every configure(), then fits the request to it: consent to persistent changes lapses where the family has
/// none, a compact-observation request lapses with a warning where the family has no compact option, and an
/// unsupported base mode fails, naming the family. Detection first asks the family of a hint (setDetectionHint()) to
/// identify itself, with the family's read-only identity query, and scans every family at every rate
/// (GPSReceiverDetector) without a hint or when that family does not answer.
class GPSDriver
{
public:
    /// The longest the identity query of a hint waits for its answer, which is shorter than some families wait when
    /// they are scanned for. A hint that no longer holds therefore costs at most this much for each of its rates
    /// before the scan starts, and a receiver that takes longer to answer is found by the scan.
    static constexpr std::chrono::milliseconds HINT_PROBE_TIMEOUT{1000};

    /// Deadlines, report times and satellite freshness run on @a clock; the default is the steady clock.
    GPSDriver(GPSType type, GPSTransport& transport, const GPSReceiverConfig& config, GPSDriverSinks sinks,
              GPSClock clock = {}, GPSDriverObserver observer = {});
    ~GPSDriver();

    Q_DISABLE_COPY(GPSDriver)

    /// Where a GPSType::Automatic driver looks for the receiver first, from the next configure() on; ignored for any
    /// other type, and when it names no family receiver detection probes. With a selected or fixed link rate the
    /// family is asked at that rate, otherwise at the hint's rates.
    void setDetectionHint(const GPSDetectionHint& hint) { _hint = hint; }

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

    /// Receiver model and firmware the latest configure() read, also when it failed after the receiver identified
    /// itself; empty when unknown.
    [[nodiscard]] QString receiverIdentity() const;

    /// The hint for the next driver on this link: the family the latest configure() of a GPSType::Automatic driver
    /// detected and configured, the rate it left the link at, and the rate it found the receiver at. A receiver found
    /// at the rate it is left at keeps the found rate of this driver's hint for the same family, which still tells
    /// where it answers after a restart. No hint, with the family GPSType::Automatic, when configure() failed or
    /// detected nothing.
    [[nodiscard]] GPSDetectionHint nextDetectionHint() const;

private:
    bool _configureDetected(GPSRuntimeIO io, GPSRuntimeObserver observer, unsigned baudRate);
    /// Asks the hint's family to identify itself, at @a baudRate when the link's rate is selected or fixed and else at
    /// the hint's rates. @return the detection when it answered; none, without an error, when there is no hint to
    /// follow or the family did not answer; none with the error when the link failed or the driver was stopped.
    [[nodiscard]] GPSReceiverDetection _detectHinted(
        const GPSRuntimeIO& io, const std::function<void(const GPSConfigurationEvidence&)>& commandFinished,
        unsigned baudRate);
    /// @a detected describes the detection that chose @a family; empty for a configured type.
    bool _configureFamily(const GPSReceiverFamily& family, GPSRuntimeIO io, GPSRuntimeObserver observer,
                          const GPSConfig& config, unsigned baudRate, const QString& detected);
    void _publish(const GPSEventBatch& batch);
    void _handle(const GPSIntegrityReport& report);
    void _handle(const GPSDecodedPosition& report);
    void _handle(const GPSDecodedSatellites& report);
    void _handle(const GPSDecodedSatelliteUsage& report);
    void _handle(const GPSBaseStatus& report);
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
    GPSDriverObserver _observer;
    GPSDetectionHint _hint;

    struct State;
    std::unique_ptr<State> _state;
};
