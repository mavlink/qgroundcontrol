#include "GPSDriver.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <optional>
#include <span>
#include <utility>
#include <variant>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QMetaEnum>

#include "GPSDecodedData.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverDetector.h"
#include "GPSReceiverFamilies.h"
#include "GPSTransport.h"
#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSDriverLog, "GPS.GPSDriver")

namespace {

/// Names the family whose traffic arrived while another one failed to configure; empty when none did.
QString mismatchHint(const GPSReceiverSignatures& signatures, GPSType configured)
{
    const auto& match = signatures.match();
    if (!match || match->family->type == configured) {
        return {};
    }
    QString evidence = match->evidence;
    evidence[0] = evidence[0].toUpper();
    return QStringLiteral("%1 were received; this looks like a %2 receiver")
        .arg(evidence, gpsReceiverName(match->family->type));
}

}  // namespace

struct GPSDriver::State
{
    GPSIntegrityReport integrity;
    GPSDecodedData::SatelliteCounts satellites;
    std::optional<GPSSatelliteReport> lastSatellites;
    std::unique_ptr<GPSProtocolRuntime> runtime;
    /// Other families' signatures in the replies to a configured type, while it configures.
    std::unique_ptr<GPSReceiverSignatures> signatures;
    std::vector<GPSConfigurationEvidence> evidence;
    QString configurationError;
    /// What the receiver of a failed configuration identified itself as; its runtime is gone.
    QString failedIdentity;
    /// What a GPSType::Automatic configuration detected and configured, as the next driver's hint.
    GPSDetectionHint detected;
    /// The rate the configured family left the link at.
    unsigned baud = 0;
    bool consentRequired = false;
    bool configuring = false;
    /// The base status published last is Valid.
    bool baseValid = false;

    struct ReceiveCycle
    {
        GPSReceiveUpdates updates;
        bool usefulData = false;
        bool activity = false;
    };

    ReceiveCycle cycle;
};

GPSDriver::GPSDriver(GPSType type, GPSTransport& transport, const GPSReceiverConfig& config, GPSDriverSinks sinks,
                     GPSClock clock, GPSDriverObserver observer)
    : _type(type)
    , _transport(transport)
    , _config(config)
    , _sinks(std::move(sinks))
    , _clock(std::move(clock))
    , _observer(std::move(observer))
    , _state(std::make_unique<State>())
{}

GPSDriver::~GPSDriver() = default;

const std::vector<GPSConfigurationEvidence>& GPSDriver::configurationEvidence() const
{
    return _state->evidence;
}

const QString& GPSDriver::configurationError() const
{
    return _state->configurationError;
}

bool GPSDriver::configurationNeedsConsent() const
{
    return _state->consentRequired;
}

QString GPSDriver::receiverIdentity() const
{
    return _state->runtime ? _state->runtime->identity() : _state->failedIdentity;
}

GPSDetectionHint GPSDriver::nextDetectionHint() const
{
    return _state->detected;
}

bool GPSDriver::configure()
{
    _state = std::make_unique<State>();
    if (const QString error = gpsReceiverConfigError(_type, _config); !error.isEmpty()) {
        _state->configurationError = error;
        qCWarning(GPSDriverLog) << error;
        return false;
    }
    if (_type != GPSType::Passive) {
        const auto& mode = _config.base.mode;
        if (std::holds_alternative<GPSBaseStationConfig::Fixed>(mode)) {
            qCDebug(GPSDriverLog) << "Fixed base configured";
        } else if (const auto* averaging = std::get_if<GPSBaseStationConfig::ReceiverAveraging>(&mode)) {
            qCDebug(GPSDriverLog) << "Receiver-managed averaging maximum duration (s):"
                                  << averaging->maximumDuration.count();
        } else if (const auto* survey = std::get_if<GPSBaseStationConfig::SurveyIn>(&mode)) {
            qCDebug(GPSDriverLog) << "Survey-in accuracy (m):" << survey->accuracyMeters
                                  << "minimum duration (s):" << survey->duration.count();
        }
    }

    GPSRuntimeIO io;
    io.clock = _clock;
    io.read = [this](std::span<uint8_t> bytes, GPSDeadline deadline) {
        auto result = _transport.read(bytes, deadline.remaining(_clock.nowUs()));
        const bool data = result.status == GPSReadStatus::Data && result.bytesRead > 0 &&
                          static_cast<size_t>(result.bytesRead) <= bytes.size();
        _state->cycle.activity |= data;
        if (data && _state->signatures && _state->configuring) {
            _state->signatures->push(bytes.first(static_cast<size_t>(result.bytesRead)));
        }
        return result;
    };
    io.write = [this](std::span<const uint8_t> bytes, GPSDeadline deadline) {
        // Do not submit another part of a multipart command after its absolute deadline.
        const auto remaining = deadline.remaining(_clock.nowUs());
        if (remaining == std::chrono::milliseconds::zero()) {
            return GPSWriteResult{GPSWriteStatus::TimedOut};
        }
        // The transport waits in real time; only the remaining time carries over from the driver's clock.
        const QDeadlineTimer transportDeadline = deadline.untilUs == UINT64_MAX
                                                     ? QDeadlineTimer(QDeadlineTimer::Forever, Qt::PreciseTimer)
                                                     : QDeadlineTimer(remaining, Qt::PreciseTimer);
        return _transport.write(bytes, transportDeadline);
    };
    io.setBaudRate = [this](unsigned baud) {
        if (_transport.setBaudRate(baud)) {
            return GPSBaudStatus::Configured;
        }
        return _transport.fatalError() ? GPSBaudStatus::Error : GPSBaudStatus::Unsupported;
    };
    io.cancelToken = _transport.cancelToken();
    GPSRuntimeObserver observer;
    observer.commandFinished = [this](const GPSConfigurationEvidence& result) {
        if (_state->configuring) {
            _state->evidence.push_back(result);
        }
        if (_observer.commandFinished) {
            _observer.commandFinished(result);
        }
    };
    observer.decoded = [this](const GPSEventBatch& batch) {
        if (_observer.decoded) {
            _observer.decoded(batch);
        }
        _publish(batch);
    };

    unsigned baudRate = _config.baudRate ? _config.baudRate : _transport.fixedBaudRate();
    if (_config.baudRate && _transport.fixedBaudRate() && _config.baudRate != _transport.fixedBaudRate()) {
        _state->configurationError = QStringLiteral("Selected baud rate differs from the transport's fixed baud rate");
        qCWarning(GPSDriverLog) << _state->configurationError;
        return false;
    }
    if (_type == GPSType::Automatic) {
        return _configureDetected(std::move(io), std::move(observer), baudRate);
    }
    const GPSReceiverFamily* family = gpsReceiverFamily(_type);
    if (!family) {
        _state->configurationError =
            QStringLiteral("Unsupported GPS type: %1")
                .arg(QLatin1StringView(QMetaEnum::fromType<GPSType>().valueToKey(static_cast<int>(_type))));
        qCWarning(GPSDriverLog) << "Unsupported GPS type:" << _type;
        return false;
    }
    // Without a selected rate or a fixed link rate, the family may prefer one to detection.
    if (family->autoBaudRate && !baudRate) {
        baudRate = family->autoBaudRate;
    }
    if (_type != GPSType::Passive) {
        _state->signatures = std::make_unique<GPSReceiverSignatures>(gpsReceiverFamilies());
    }
    return _configureFamily(*family, std::move(io), std::move(observer),
                            {.base = _config.base, .allowPersistentChanges = _config.allowPersistentChanges}, baudRate,
                            {});
}

GPSReceiverDetection GPSDriver::_detectHinted(
    const GPSRuntimeIO& io, const std::function<void(const GPSConfigurationEvidence&)>& commandFinished,
    unsigned baudRate)
{
    GPSReceiverDetection detection;
    const GPSReceiverFamily* const family =
        _hint.family == GPSType::Automatic ? nullptr : gpsReceiverFamily(_hint.family);
    if (!family || family->baudCandidates.empty()) {
        return detection;
    }
    // The rate the receiver was left at first: a receiver that restarted since answers at the rate it was found at.
    const std::array<unsigned, 2> rates =
        baudRate ? std::array{baudRate, 0U}
                 : std::array{_hint.baud, _hint.foundBaud == _hint.baud ? 0U : _hint.foundBaud};
    for (const unsigned rate : rates) {
        // A rate the link cannot run at is left to the scan, which reports it.
        if (!rate || io.setBaudRate(rate) != GPSBaudStatus::Configured) {
            continue;
        }
        if (_observer.detecting) {
            _observer.detecting(family->type, rate, true);
        }
        GPSProtocolRuntime runtime(*family, io, {.commandFinished = commandFinished});
        const bool answered = runtime.probe(HINT_PROBE_TIMEOUT);
        if (runtime.error() == GPSProtocolError::Cancelled || runtime.error() == GPSProtocolError::Transport) {
            detection.error = runtime.error();
            detection.errorDetail = runtime.errorDetail();
            return detection;
        }
        if (answered) {
            qCDebug(GPSDriverLog).noquote() << QStringLiteral("%1 receiver answered at %2 baud, where it was expected")
                                                   .arg(gpsReceiverName(family->type))
                                                   .arg(rate);
            detection.family = family;
            detection.baud = rate;
            detection.evidence = QStringLiteral("identity query");
            return detection;
        }
    }
    return detection;
}

bool GPSDriver::_configureDetected(GPSRuntimeIO io, GPSRuntimeObserver observer, unsigned baudRate)
{
    _state->configuring = true;
    GPSReceiverDetection detection = _detectHinted(io, observer.commandFinished, baudRate);
    if (!detection.found() && detection.error == GPSProtocolError::None) {
        GPSReceiverDetector::Looking looking;
        if (_observer.detecting) {
            looking = [this](GPSType family, unsigned baud) { _observer.detecting(family, baud, false); };
        }
        detection = GPSReceiverDetector(gpsReceiverFamilies(), io, observer.commandFinished, std::move(looking))
                        .detect(baudRate);
    }
    _state->configuring = false;
    if (_observer.detected) {
        _observer.detected(detection, baudRate);
    }
    if (!detection.found()) {
        _state->configurationError = !detection.errorDetail.isEmpty() ? detection.errorDetail
                                     : detection.error == GPSProtocolError::Cancelled
                                         ? QStringLiteral("Receiver detection cancelled")
                                         : QStringLiteral("No supported receiver was identified");
        qCWarning(GPSDriverLog) << "Receiver detection failed:" << _state->configurationError;
        return false;
    }
    const GPSType detected = detection.family->type;
    const QString name = gpsReceiverName(detected);
    if (_sinks.onReceiverDetected) {
        _sinks.onReceiverDetected(detected);
    }
    bool compactFallback = false;
    bool constellationFallback = false;
    const GPSReceiverConfig config =
        gpsReceiverConfigForDetected(detected, _config, &compactFallback, &constellationFallback);
    if (compactFallback) {
        qCWarning(GPSDriverLog).noquote()
            << QStringLiteral("Detected %1 receiver has no compact (MSM4) RTCM option; it sends its standard set")
                   .arg(name);
    }
    if (constellationFallback) {
        qCWarning(GPSDriverLog).noquote()
            << QStringLiteral("Detected %1 receiver has no choice of RTCM constellations; it sends its standard set")
                   .arg(name);
    }
    if (const QString error = gpsDetectedReceiverConfigError(detected, config); !error.isEmpty()) {
        _state->configurationError = error;
        qCWarning(GPSDriverLog).noquote() << error;
        return false;
    }
    // Without a fixed or selected rate, the family's baud search starts at the detected rate and moves the link to
    // the rate the family prefers.
    if (!_configureFamily(*detection.family, std::move(io), std::move(observer),
                          {.base = config.base,
                           .allowPersistentChanges = config.allowPersistentChanges,
                           .detectedBaud = detection.baud},
                          baudRate, QStringLiteral("Detected %1 receiver at %2 baud").arg(name).arg(detection.baud))) {
        return false;
    }
    // A receiver found at the rate it is left at either kept running or starts at that rate. The rate an earlier
    // driver found it at then still tells where it answers after a restart, unless the link has only one rate.
    const bool keepsFoundBaud =
        !baudRate && detection.baud == _state->baud && _hint.family == detected && _hint.foundBaud != 0;
    _state->detected = {
        .family = detected, .baud = _state->baud, .foundBaud = keepsFoundBaud ? _hint.foundBaud : detection.baud};
    return true;
}

bool GPSDriver::_configureFamily(const GPSReceiverFamily& family, GPSRuntimeIO io, GPSRuntimeObserver observer,
                                 const GPSConfig& config, unsigned baudRate, const QString& detected)
{
    _state->runtime = std::make_unique<GPSProtocolRuntime>(family, std::move(io), std::move(observer));
    _state->configuring = true;
    const unsigned requestedBaud = baudRate;
    const bool configured = _state->runtime->configure(config, baudRate);
    _state->configuring = false;
    if (_observer.configured) {
        _observer.configured(*_state->runtime, requestedBaud, baudRate);
    }
    const auto signatures = std::exchange(_state->signatures, nullptr);
    if (!configured) {
        QString error = _state->runtime->errorDetail();
        if (error.isEmpty()) {
            error = QStringLiteral("Receiver configuration failed");
        }
        if (!detected.isEmpty()) {
            error = QStringLiteral("%1: %2").arg(detected, error);
        } else if (const QString hint = signatures ? mismatchHint(*signatures, family.type) : QString();
                   !hint.isEmpty()) {
            error += (error.endsWith(u'.') ? QStringLiteral(" ") : QStringLiteral(". ")) + hint;
        }
        _state->configurationError = error;
        _state->consentRequired = _state->runtime->error() == GPSProtocolError::ConsentRequired;
        _state->failedIdentity = _state->runtime->identity();
        qCWarning(GPSDriverLog) << "Driver configuration failed for type" << family.type << error;
        _state->runtime.reset();
        return false;
    }
    _state->configurationError.clear();
    _state->baud = baudRate;
    return true;
}

GPSReceiveResult GPSDriver::receiveOutcome(std::chrono::milliseconds timeout)
{
    if (!_state->runtime) {
        return {GPSProtocolError::InvalidArgument};
    }
    _state->cycle = {};
    _publishExpiredSatellites();
    const GPSReceiveUpdates decoded = _state->runtime->receive(timeout);
    _publishExpiredSatellites();
    if (_state->runtime->error() != GPSProtocolError::None) {
        return {.error = _state->runtime->error(),
                .updates = _state->cycle.updates,
                .detail = _state->runtime->errorDetail()};
    }
    if (_transport.isCancelled()) {
        return {.error = GPSProtocolError::Cancelled, .updates = _state->cycle.updates};
    }
    if (_transport.fatalError()) {
        // An empty read reports why the link failed without consuming input.
        const GPSReadResult failure = _transport.read({}, std::chrono::milliseconds::zero());
        return {.error = GPSProtocolError::Transport, .updates = _state->cycle.updates, .detail = failure.detail};
    }
    return {.liveness = _state->cycle.usefulData                                   ? GPSReceiveLiveness::Data
                        : _state->cycle.activity || decoded != GPSReceiveUpdates{} ? GPSReceiveLiveness::Activity
                                                                                   : GPSReceiveLiveness::Idle,
            .updates = _state->cycle.updates};
}

void GPSDriver::_publish(const GPSEventBatch& batch)
{
    _state->cycle.activity |= !batch.events.empty();
    for (const auto& event : batch.events) {
        std::visit([this](const auto& report) { _handle(report); }, event);
    }
}

void GPSDriver::_handle(const GPSIntegrityReport& report)
{
    _state->integrity = report;
}

void GPSDriver::_handle(const GPSDecodedPosition& report)
{
    if (_state->configuring) {
        return;
    }
    _state->cycle.usefulData = true;
    _state->cycle.updates |= GPSReceiveUpdate::Position;
    if (_sinks.onPosition) {
        _sinks.onPosition(GPSDecodedData::position(report, _state->integrity, _clock.nowUs()));
    }
}

void GPSDriver::_handle(const GPSDecodedSatellites& report)
{
    if (!_state->configuring) {
        _handleSatellites(_state->satellites.update(report, _clock.nowUs()));
    }
}

void GPSDriver::_handle(const GPSDecodedSatelliteUsage& report)
{
    if (!_state->configuring) {
        _handleSatellites(_state->satellites.update(report, _clock.nowUs()));
    }
}

void GPSDriver::_handleSatellites(const GPSSatelliteReport& report)
{
    _state->cycle.usefulData = true;
    _state->cycle.updates |= GPSReceiveUpdate::Satellites;
    _publishSatellites(report);
}

void GPSDriver::_handle(const GPSBaseStatus& report)
{
    _state->cycle.usefulData = true;
    _state->baseValid = report.state == GPSBaseState::Valid;
    if (_sinks.onBaseStatus) {
        _sinks.onBaseStatus(report);
    }
}

void GPSDriver::_handle(const GPSRTCMFrame& report)
{
    // Corrections state the base position, so those of a base configured here are forwarded only while it reports
    // that position valid.
    if (_state->configuring ||
        (!_state->baseValid && !forwardsCorrectionsWithoutBase(_state->runtime->family().type))) {
        return;
    }
    _state->cycle.usefulData = true;
    if (_sinks.onRTCM) {
        _sinks.onRTCM(report.bytes);
    }
}

void GPSDriver::_handle(const GPSInputProtocol& report)
{
    // Satellite counts come from one protocol, as positions do. The counts of the protocol left behind would otherwise
    // stay with the consumer, since nothing remains here to expire.
    _state->satellites = {};
    if (_state->lastSatellites) {
        _publishSatellites({});
    }
    if (_sinks.onReceiverDetected) {
        _sinks.onReceiverDetected(report.family);
    }
}

void GPSDriver::_publishExpiredSatellites()
{
    if (const auto expired = _state->satellites.expire(_clock.nowUs())) {
        // Cache retirement is a notification, not new receiver traffic or navigation liveness.
        _publishSatellites(*expired);
    }
}

void GPSDriver::_publishSatellites(const GPSSatelliteReport& report)
{
    // Count-only usage can repeat every fix; consumers only need changed counts.
    if (_state->lastSatellites == report) {
        return;
    }
    _state->lastSatellites = report;
    if (_sinks.onSatelliteInfo) {
        _sinks.onSatelliteInfo(report);
    }
}
