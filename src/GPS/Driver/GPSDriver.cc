#include "GPSDriver.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <thread>
#include <type_traits>
#include <utility>

#include <QtCore/QScopedValueRollback>

#include "Ashtech/GPSDriverAshtech.h"
#include "Femto/GPSDriverFemto.h"
#include "GPSAsciiProtocol.h"
#include "GPSNativeData_p.h"
#include "GPSProtocol.h"
#include "GPSTransport.h"
#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"
#include "Quectel/GPSDriverQuectel.h"
#include "SBF/GPSDriverSBF.h"
#include "UBX/GPSDriverUBX.h"
#include "Unicore/GPSDriverUnicore.h"

QGC_LOGGING_CATEGORY(GPSDriverLog, "GPS.Driver.GPSDriver")

namespace {
template <typename Protocol>
std::unique_ptr<GPSProtocol> makeProtocol(GPSProtocolIO io)
{
    return std::make_unique<Protocol>(std::move(io));
}

struct ProtocolFactory
{
    GPSType type;
    std::unique_ptr<GPSProtocol> (*create)(GPSProtocolIO);
    unsigned autoBaudRate = 0;
};

constexpr std::array PROTOCOL_FACTORIES{
    ProtocolFactory{GPSType::ublox, &makeProtocol<GPSNativeUBX>},
    ProtocolFactory{GPSType::trimble, &makeProtocol<GPSNativeAshtech>, 115200},
    ProtocolFactory{GPSType::septentrio, &makeProtocol<GPSNativeSBF>},
    ProtocolFactory{GPSType::femto, &makeProtocol<GPSNativeFemto>},
    ProtocolFactory{GPSType::unicore, &makeProtocol<GPSNativeUnicore>},
    ProtocolFactory{GPSType::quectel, &makeProtocol<GPSNativeQuectel>},
    ProtocolFactory{GPSType::passive, &makeProtocol<GPSNativePassive>},
};

auto findProtocolFactory(GPSType type)
{
    return std::find_if(PROTOCOL_FACTORIES.begin(), PROTOCOL_FACTORIES.end(),
                        [type](const ProtocolFactory& factory) { return factory.type == type; });
}
}  // namespace

struct GPSDriver::State
{
    GPSIntegrityReport integrity;
    GPSNativeData::SatelliteSnapshot satelliteSnapshot;
    std::optional<GPSSatelliteReport> lastSatellites;
    std::unique_ptr<GPSProtocol> driver;
    std::vector<GPSConfigurationEvidence> evidence;
    QString configurationError;
    bool configuring = false;

    struct ReceiveCycle
    {
        int updates = 0;
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

QString GPSDriver::receiverIdentity() const
{
    return _state->driver ? QString::fromStdString(_state->driver->receiverIdentity()).trimmed() : QString();
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

    GPSProtocolIO io;
    io.nowUs = MonotonicClock::nowUs;
    io.read = [this](std::span<uint8_t> bytes, GPSDeadline deadline) {
        const int timeout = deadline.remainingMilliseconds(MonotonicClock::nowUs());
        const auto result = _transport.read(bytes.data(), static_cast<int>(bytes.size()), timeout);
        _state->cycle.activity |= result.status == GPSReadStatus::Data && result.bytesRead > 0;
        return result;
    };
    io.write = [this](std::span<const uint8_t> bytes, GPSDeadline deadline) {
        const int remaining = deadline.remainingMilliseconds(MonotonicClock::nowUs());
        // Do not submit another part of a multipart command after its absolute deadline.
        if (_transport.isCancelled() || remaining == 0) {
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
    io.log = [](const QLoggingCategory& category, GPSProtocolLogLevel level, QStringView message) {
        const QMessageLogger logger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC);
        if (level == GPSProtocolLogLevel::Debug) {
            logger.debug(category) << message;
        } else {
            logger.warning(category) << message;
        }
    };
    io.commandFinished = [this](const GPSCommandResult& result) {
        if (_state->configuring) {
            _state->evidence.push_back(result.evidence);
        }
    };
    io.decoded = [this](const GPSDecodedBatch& batch) {
        _state->cycle.activity |= !batch.events.empty();
        for (const auto& event : batch.events) {
            std::visit(
                [this](const auto& report) {
                    using Report = std::decay_t<decltype(report)>;
                    if constexpr (std::is_same_v<Report, GPSIntegrityReport>) {
                        _state->integrity = report;
                    } else if constexpr (std::is_same_v<Report, GPSNativePositionReport>) {
                        if (!_state->configuring) {
                            _state->cycle.usefulData = true;
                            _state->cycle.updates |= GPSReceiveResult::POSITION_UPDATE;
                            if (_sinks.onPosition) {
                                _sinks.onPosition(
                                    GPSNativeData::position(report, _state->integrity, MonotonicClock::nowUs()));
                            }
                        }
                    } else if constexpr (std::is_same_v<Report, GPSNativeSatelliteReport>) {
                        if (!_state->configuring) {
                            _state->cycle.usefulData = true;
                            _state->cycle.updates |= GPSReceiveResult::SATELLITES_UPDATE;
                            _publishSatellites(_state->satelliteSnapshot.update(report, MonotonicClock::nowUs()));
                        }
                    } else if constexpr (std::is_same_v<Report, GPSNativeSatelliteUsageReport>) {
                        if (!_state->configuring) {
                            _state->cycle.usefulData = true;
                            _state->cycle.updates |= GPSReceiveResult::SATELLITES_UPDATE;
                            _publishSatellites(_state->satelliteSnapshot.update(report, MonotonicClock::nowUs()));
                        }
                    } else if constexpr (std::is_same_v<Report, GPSNativeSurveyReport>) {
                        _state->cycle.usefulData = true;
                        if (_sinks.onSurveyIn) {
                            _sinks.onSurveyIn(report.survey);
                        }
                    } else if constexpr (std::is_same_v<Report, GPSRTCMReport>) {
                        _state->cycle.usefulData = true;
                        if (!_state->configuring && _sinks.onRTCM) {
                            _sinks.onRTCM(std::span(report.bytes).first(report.size));
                        }
                    }
                },
                event);
        }
    };

    unsigned baudrate = _config.baudRate ? _config.baudRate : _transport.fixedBaudrate();
    if (_config.baudRate && _transport.fixedBaudrate() && _config.baudRate != _transport.fixedBaudrate()) {
        _state->configurationError = QStringLiteral("Selected baud rate differs from the transport's fixed baud rate");
        qCWarning(GPSDriverLog) << _state->configurationError;
        return false;
    }
    const auto factory = findProtocolFactory(_type);
    if (factory == PROTOCOL_FACTORIES.end()) {
        _state->configurationError = QStringLiteral("Unsupported GPS type: %1").arg(static_cast<int>(_type));
        qCWarning(GPSDriverLog) << "Unsupported GPS type:" << static_cast<int>(_type);
        return false;
    }
    _state->driver = factory->create(std::move(io));
    if (factory->autoBaudRate && !_config.baudRate) {
        baudrate = factory->autoBaudRate;
    }
    GPSProtocol::GPSConfig config{};
    config.base = _config.base;
    config.allowPersistentChanges = _config.allowPersistentChanges;
    _state->configuring = true;
    const bool configured = _state->driver->configure(baudrate, config);
    _state->driver->finishConfigurationEvidence();
    if (configured) {
        // Configuration can finish by publishing a fixed-base or survey-start event without another read.
        _state->driver->consume({});
    }
    _state->configuring = false;
    if (!configured) {
        _state->configurationError = _state->driver->ioErrorDetail();
        if (_state->configurationError.isEmpty()) {
            _state->configurationError = QStringLiteral("Receiver configuration failed");
        }
        qCWarning(GPSDriverLog) << "Driver configuration failed for type" << static_cast<int>(_type)
                                << _state->configurationError;
        _state->driver.reset();
        return false;
    }
    _state->configurationError.clear();
    return true;
}

GPSReceiveResult GPSDriver::receiveOutcome(unsigned timeoutMs)
{
    if (_operationInProgress) {
        const QString detail = QStringLiteral("Receiver operation already in progress; receive rejected");
        qCWarning(GPSDriverLog) << detail;
        return {GPSReceiveStatus::Busy, 0, detail};
    }
    const QScopedValueRollback operation(_operationInProgress, true);
    if (!_state->driver) {
        return {GPSReceiveStatus::NotConfigured, 0};
    }
    _state->cycle = {};
    _publishExpiredSatellites();
    const int result = _state->driver->receive(timeoutMs);
    _publishExpiredSatellites();
    switch (_state->driver->ioError()) {
        case GPSProtocolError::None:
            break;
        case GPSProtocolError::Cancelled:
            return {GPSReceiveStatus::Cancelled, _state->cycle.updates, _state->driver->ioErrorDetail()};
        case GPSProtocolError::Protocol:
            return {GPSReceiveStatus::ProtocolError, _state->cycle.updates, _state->driver->ioErrorDetail()};
        case GPSProtocolError::Transport:
        case GPSProtocolError::InvalidArgument:
            return {GPSReceiveStatus::TransportError, _state->cycle.updates, _state->driver->ioErrorDetail()};
    }
    if (_transport.isCancelled()) {
        return {GPSReceiveStatus::Cancelled, _state->cycle.updates};
    }
    if (_transport.fatalError()) {
        return {GPSReceiveStatus::TransportError, _state->cycle.updates};
    }
    return {_state->cycle.usefulData               ? GPSReceiveStatus::Data
            : _state->cycle.activity || result > 0 ? GPSReceiveStatus::Activity
                                                   : GPSReceiveStatus::Idle,
            _state->cycle.updates};
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
