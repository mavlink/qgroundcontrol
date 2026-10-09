#include "GPSDriver.h"

#include <algorithm>
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
    bool consentRequired = false;
    bool configuring = false;

    struct ReceiveCycle
    {
        GPSReceiveUpdates updates;
        bool usefulData = false;
        bool activity = false;
    };

    ReceiveCycle cycle;
};

GPSDriver::GPSDriver(GPSType type, GPSTransport& transport, const GPSReceiverConfig& config, GPSDriverSinks sinks,
                     GPSClock clock)
    : _type(type)
    , _transport(transport)
    , _config(config)
    , _sinks(std::move(sinks))
    , _clock(std::move(clock))
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
    return _state->runtime ? _state->runtime->identity() : QString();
}

bool GPSDriver::configure()
{
    _state = std::make_unique<State>();
    if (const QString error = gpsReceiverConfigError(_type, _config); !error.isEmpty()) {
        _state->configurationError = error;
        qCWarning(GPSDriverLog) << error;
        return false;
    }
    if (_type != GPSType::passive) {
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
        const auto result = _transport.read(bytes, deadline.remaining(_clock.nowUs()));
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
    io.setBaudrate = [this](unsigned baud) {
        if (_transport.setBaudrate(baud)) {
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
    };
    observer.decoded = [this](const GPSEventBatch& batch) { _publish(batch); };

    unsigned baudrate = _config.baudRate ? _config.baudRate : _transport.fixedBaudrate();
    if (_config.baudRate && _transport.fixedBaudrate() && _config.baudRate != _transport.fixedBaudrate()) {
        _state->configurationError = QStringLiteral("Selected baud rate differs from the transport's fixed baud rate");
        qCWarning(GPSDriverLog) << _state->configurationError;
        return false;
    }
    if (_type == GPSType::automatic) {
        return _configureDetected(std::move(io), std::move(observer), baudrate);
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
    if (family->autoBaudRate && !baudrate) {
        baudrate = family->autoBaudRate;
    }
    if (_type != GPSType::passive) {
        _state->signatures = std::make_unique<GPSReceiverSignatures>(gpsReceiverFamilies());
    }
    return _configureFamily(*family, std::move(io), std::move(observer),
                            {.base = _config.base, .allowPersistentChanges = _config.allowPersistentChanges}, baudrate,
                            {});
}

bool GPSDriver::_configureDetected(GPSRuntimeIO io, GPSRuntimeObserver observer, unsigned baudrate)
{
    _state->configuring = true;
    const GPSReceiverDetection detection =
        GPSReceiverDetector(gpsReceiverFamilies(), io, observer.commandFinished).detect(baudrate);
    _state->configuring = false;
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
    const GPSReceiverConfig config = gpsReceiverConfigForDetected(detected, _config, &compactFallback);
    if (compactFallback) {
        qCWarning(GPSDriverLog).noquote()
            << QStringLiteral("Detected %1 receiver has no compact (MSM4) RTCM option; it sends its standard set")
                   .arg(name);
    }
    if (const QString error = gpsDetectedReceiverConfigError(detected, config); !error.isEmpty()) {
        _state->configurationError = error;
        qCWarning(GPSDriverLog).noquote() << error;
        return false;
    }
    // Without a fixed or selected rate, the family's baud search starts at the detected rate and moves the link to
    // the rate the family prefers.
    return _configureFamily(
        *detection.family, std::move(io), std::move(observer),
        {.base = config.base, .allowPersistentChanges = config.allowPersistentChanges, .detectedBaud = detection.baud},
        baudrate, QStringLiteral("Detected %1 receiver at %2 baud").arg(name).arg(detection.baud));
}

bool GPSDriver::_configureFamily(const GPSReceiverFamily& family, GPSRuntimeIO io, GPSRuntimeObserver observer,
                                 const GPSConfig& config, unsigned baudrate, const QString& detected)
{
    _state->runtime = std::make_unique<GPSProtocolRuntime>(family, std::move(io), std::move(observer));
    _state->configuring = true;
    const bool configured = _state->runtime->configure(config, baudrate);
    _state->configuring = false;
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
        qCWarning(GPSDriverLog) << "Driver configuration failed for type" << family.type << error;
        _state->runtime.reset();
        return false;
    }
    _state->configurationError.clear();
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

void GPSDriver::_handle(const GPSSurveyReport& report)
{
    _state->cycle.usefulData = true;
    if (_sinks.onSurveyIn) {
        _sinks.onSurveyIn(report);
    }
}

void GPSDriver::_handle(const GPSRTCMFrame& report)
{
    _state->cycle.usefulData = true;
    if (!_state->configuring && _sinks.onRTCM) {
        _sinks.onRTCM(report.bytes);
    }
}

void GPSDriver::_handle(const GPSInputProtocol& report)
{
    // Satellite counts come from one protocol, as positions do.
    _state->satellites = {};
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
