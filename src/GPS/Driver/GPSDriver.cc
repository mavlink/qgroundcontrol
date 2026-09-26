#include "GPSDriver.h"

#include <algorithm>
#include <chrono>
#include <thread>
#include <type_traits>
#include <utility>

#include <QtCore/QScopedValueRollback>

#include "GPSDecodedData_p.h"
#include "GPSProtocolRuntime.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverDetector.h"
#include "GPSReceiverFamilies.h"
#include "GPSTransport.h"
#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSDriverLog, "GPS.Driver.GPSDriver")

namespace {

QString receiverName(GPSType type)
{
    const auto* descriptor = gpsReceiverDescriptor(type);
    return descriptor ? QString::fromLatin1(descriptor->name.data(), static_cast<qsizetype>(descriptor->name.size()))
                      : QString();
}

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
        .arg(evidence, receiverName(match->family->type));
}

}  // namespace

bool GPSDriver::supportsType(GPSType type)
{
    return type == GPSType::automatic || gpsReceiverFamily(type) != nullptr;
}

struct GPSDriver::State
{
    GPSIntegrityReport integrity;
    GPSDecodedData::SatelliteSnapshot satelliteSnapshot;
    std::optional<GPSSatelliteReport> lastSatellites;
    std::unique_ptr<GPSProtocolRuntime> runtime;
    /// Other families' signatures in the replies to a configured type, while it configures.
    std::unique_ptr<GPSReceiverSignatures> signatures;
    std::vector<GPSConfigurationEvidence> evidence;
    QString configurationError;
    std::optional<GPSType> detectedType;
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

GPSDriver::GPSDriver(GPSType type, GPSTransport& transport, const GPSReceiverConfig& config, GPSDriverSinks sinks)
    : _type(type)
    , _transport(transport)
    , _config(config)
    , _sinks(std::move(sinks))
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

std::optional<GPSType> GPSDriver::detectedType() const
{
    return _state->detectedType;
}

bool GPSDriver::configure()
{
    if (_operationInProgress) {
        _state->configurationError = QStringLiteral("Receiver operation already in progress; configuration rejected");
        qCWarning(GPSDriverLog) << _state->configurationError;
        return false;
    }
    const QScopedValueRollback operation(_operationInProgress, true);
    _state = std::make_unique<State>();
    if (const QString error = gpsReceiverConfigError(_type, _config); !error.isEmpty()) {
        _state->configurationError = error;
        qCWarning(GPSDriverLog) << error;
        return false;
    }

    GPSRuntimeIO io;
    io.nowUs = MonotonicClock::nowUs;
    io.read = [this](std::span<uint8_t> bytes, GPSDeadline deadline) {
        const auto result =
            _transport.read(bytes.data(), static_cast<int>(bytes.size()), deadline.remaining(MonotonicClock::nowUs()));
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
        if (_transport.isCancelled() ||
            deadline.remaining(MonotonicClock::nowUs()) == std::chrono::milliseconds::zero()) {
            return GPSWriteResult{_transport.isCancelled() ? GPSWriteStatus::Cancelled : GPSWriteStatus::TimedOut};
        }
        return _transport.write(bytes.data(), static_cast<int>(bytes.size()), deadline.toQDeadlineTimer());
    };
    io.setBaudrate = [this](unsigned baud) {
        if (_transport.isCancelled()) {
            return GPSBaudStatus::Cancelled;
        }
        return _transport.setBaudrate(baud) ? GPSBaudStatus::Configured : GPSBaudStatus::Error;
    };
    io.wait = [this](std::chrono::microseconds duration) {
        const auto until = std::chrono::steady_clock::now() + duration;
        while (!_transport.isCancelled() && std::chrono::steady_clock::now() < until) {
            std::this_thread::sleep_for(std::min(until - std::chrono::steady_clock::now(),
                                                 std::chrono::steady_clock::duration(std::chrono::milliseconds(20))));
        }
        return !_transport.isCancelled();
    };
    io.isCancelled = [this] { return _transport.isCancelled(); };
    GPSRuntimeObserver observer;
    observer.commandFinished = [this](const GPSCommandResult& result) {
        if (_state->configuring) {
            _state->evidence.push_back(result.evidence);
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
        _state->configurationError = QStringLiteral("Unsupported GPS type: %1").arg(static_cast<int>(_type));
        qCWarning(GPSDriverLog) << "Unsupported GPS type:" << _type;
        return false;
    }
    if (family->autoBaudRate && !_config.baudRate) {
        baudrate = family->autoBaudRate;
    }
    if (_config.role == GPSReceiverConfig::Role::RTKBase) {
        _state->signatures = std::make_unique<GPSReceiverSignatures>(gpsReceiverFamilies());
    }
    return _configureFamily(*family, std::move(io), std::move(observer),
                            {.base = _config.base, .allowPersistentChanges = _config.allowPersistentChanges}, baudrate,
                            {});
}

bool GPSDriver::_configureDetected(GPSRuntimeIO io, GPSRuntimeObserver observer, unsigned baudrate)
{
    GPSRuntimeObserver probes;
    probes.commandFinished = observer.commandFinished;
    _state->configuring = true;
    const GPSReceiverDetection detection =
        GPSReceiverDetector(gpsReceiverFamilies(), io, std::move(probes)).detect(baudrate);
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
    const QString name = receiverName(detected);
    _state->detectedType = detected;
    qCDebug(GPSDriverLog).noquote() << QStringLiteral("Detected %1 receiver at %2 baud from %3")
                                           .arg(name)
                                           .arg(detection.baud)
                                           .arg(detection.evidence);
    if (_sinks.onReceiverDetected) {
        _sinks.onReceiverDetected(detected);
    }
    bool compactFallback = false;
    const GPSReceiverConfig config = gpsReceiverConfigForDetected(detected, _config, &compactFallback);
    if (compactFallback) {
        qCWarning(GPSDriverLog).noquote()
            << QStringLiteral("Detected %1 receiver cannot send compact (MSM4) RTCM corrections; sending full MSM7")
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
    if (_operationInProgress) {
        const QString detail = QStringLiteral("Receiver operation already in progress; receive rejected");
        qCWarning(GPSDriverLog) << detail;
        return {GPSReceiveStatus::Busy, {}, detail};
    }
    const QScopedValueRollback operation(_operationInProgress, true);
    if (!_state->runtime) {
        return {GPSReceiveStatus::NotConfigured};
    }
    _state->cycle = {};
    _publishExpiredSatellites();
    const GPSReceiveUpdates decoded = _state->runtime->receive(timeout);
    _publishExpiredSatellites();
    switch (_state->runtime->error()) {
        case GPSProtocolError::None:
            break;
        case GPSProtocolError::Cancelled:
            return {GPSReceiveStatus::Cancelled, _state->cycle.updates, _state->runtime->errorDetail()};
        case GPSProtocolError::Protocol:
        case GPSProtocolError::ConsentRequired:
            return {GPSReceiveStatus::ProtocolError, _state->cycle.updates, _state->runtime->errorDetail()};
        case GPSProtocolError::Transport:
        case GPSProtocolError::InvalidArgument:
            return {GPSReceiveStatus::TransportError, _state->cycle.updates, _state->runtime->errorDetail()};
    }
    if (_transport.isCancelled()) {
        return {GPSReceiveStatus::Cancelled, _state->cycle.updates};
    }
    if (_transport.fatalError()) {
        return {GPSReceiveStatus::TransportError, _state->cycle.updates};
    }
    return {_state->cycle.usefulData                                   ? GPSReceiveStatus::Data
            : _state->cycle.activity || decoded != GPSReceiveUpdates{} ? GPSReceiveStatus::Activity
                                                                       : GPSReceiveStatus::Idle,
            _state->cycle.updates};
}

void GPSDriver::_publish(const GPSEventBatch& batch)
{
    _state->cycle.activity |= !batch.events.empty();
    for (const auto& event : batch.events) {
        std::visit(
            [this](const auto& report) {
                using Report = std::decay_t<decltype(report)>;
                if constexpr (std::is_same_v<Report, GPSIntegrityReport>) {
                    _state->integrity = report;
                } else if constexpr (std::is_same_v<Report, GPSDecodedPosition>) {
                    if (!_state->configuring) {
                        _state->cycle.usefulData = true;
                        _state->cycle.updates |= GPSReceiveUpdate::Position;
                        if (_sinks.onPosition) {
                            _sinks.onPosition(
                                GPSDecodedData::position(report, _state->integrity, MonotonicClock::nowUs()));
                        }
                    }
                } else if constexpr (std::is_same_v<Report, GPSDecodedSatellites> ||
                                     std::is_same_v<Report, GPSDecodedSatelliteUsage>) {
                    if (!_state->configuring) {
                        _state->cycle.usefulData = true;
                        _state->cycle.updates |= GPSReceiveUpdate::Satellites;
                        _publishSatellites(_state->satelliteSnapshot.update(report, MonotonicClock::nowUs()));
                    }
                } else if constexpr (std::is_same_v<Report, GPSDecodedSurvey>) {
                    _state->cycle.usefulData = true;
                    if (_sinks.onSurveyIn) {
                        _sinks.onSurveyIn(report.survey);
                    }
                } else if constexpr (std::is_same_v<Report, GPSRTCMFrame>) {
                    _state->cycle.usefulData = true;
                    if (!_state->configuring && _sinks.onRTCM) {
                        _sinks.onRTCM(report.bytes);
                    }
                }
            },
            event);
    }
}

void GPSDriver::_publishExpiredSatellites()
{
    if (const auto expired = _state->satelliteSnapshot.expire(MonotonicClock::nowUs())) {
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
