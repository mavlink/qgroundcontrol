#include "GPSDriver.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <thread>
#include <type_traits>
#include <utility>

#include <QtCore/QScopedValueRollback>

#include "GPSNativeData_p.h"
#include "GPSProtocol.h"
#include "GPSProtocolFeatures.h"
#include "GPSReceiverConfigValidation.h"
#include "GPSTransport.h"
#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"

#if QGC_GPS_ENABLE_UBX
#include "UBX/GPSDriverUBX.h"
#endif
#if QGC_GPS_ENABLE_ASHTECH
#include "Ashtech/GPSDriverAshtech.h"
#endif
#if QGC_GPS_ENABLE_SBF
#include "SBF/GPSDriverSBF.h"
#endif
#if QGC_GPS_ENABLE_FEMTO
#include "Femto/GPSDriverFemto.h"
#endif
#if QGC_GPS_ENABLE_UNICORE
#include "Unicore/GPSDriverUnicore.h"
#endif
#if QGC_GPS_ENABLE_QUECTEL
#include "Quectel/GPSDriverQuectel.h"
#endif
#if QGC_GPS_ENABLE_PASSIVE
#include "Passive/GPSDriverPassive.h"
#endif

QGC_LOGGING_CATEGORY(GPSDriverLog, "GPS.GPSDriver")
QGC_LOGGING_CATEGORY(GPSNativeDriversLog, "GPS.Drivers")

namespace {
template <typename Protocol>
std::unique_ptr<GPSProtocol> makeProtocol(GPSProtocolIO io, GPSNativePositionReport* position,
                                          GPSNativeSatelliteReport* satellites)
{
    return std::make_unique<Protocol>(std::move(io), position, satellites);
}

struct ProtocolFactory
{
    GPSType type;
    std::unique_ptr<GPSProtocol> (*create)(GPSProtocolIO, GPSNativePositionReport*, GPSNativeSatelliteReport*);
};

constexpr std::array PROTOCOL_FACTORIES{
#if QGC_GPS_ENABLE_UBX
    ProtocolFactory{GPSType::ublox, &makeProtocol<GPSNativeUBX>},
#endif
#if QGC_GPS_ENABLE_ASHTECH
    ProtocolFactory{GPSType::trimble, &makeProtocol<GPSNativeAshtech>},
#endif
#if QGC_GPS_ENABLE_SBF
    ProtocolFactory{GPSType::septentrio, &makeProtocol<GPSNativeSBF>},
#endif
#if QGC_GPS_ENABLE_FEMTO
    ProtocolFactory{GPSType::femto, &makeProtocol<GPSNativeFemto>},
#endif
#if QGC_GPS_ENABLE_UNICORE
    ProtocolFactory{GPSType::unicore, &makeProtocol<GPSNativeUnicore>},
#endif
#if QGC_GPS_ENABLE_QUECTEL
    ProtocolFactory{GPSType::quectel, &makeProtocol<GPSNativeQuectel>},
#endif
#if QGC_GPS_ENABLE_PASSIVE
    ProtocolFactory{GPSType::passive, &makeProtocol<GPSNativePassive>},
#endif
};

auto findProtocolFactory(GPSType type)
{
    return std::find_if(PROTOCOL_FACTORIES.begin(), PROTOCOL_FACTORIES.end(),
                        [type](const ProtocolFactory& factory) { return factory.type == type; });
}
}  // namespace

struct GPSDriver::State
{
    GPSNativePositionReport position;
    GPSNativeSatelliteReport satellites;
    GPSNativeIntegrityReport integrity;
    GPSNativeData::SatelliteSnapshot satelliteSnapshot;
    std::unique_ptr<GPSProtocol> driver;
    std::vector<GPSConfigurationEvidence> evidence;
    QString configurationError;
    bool configuring = false;
    int updates = 0;
    bool usefulData = false;
    bool activity = false;
};

GPSDriver::GPSDriver(GPSType type, GPSTransport& transport, const GPSReceiverConfig& config, GPSDriverSinks sinks)
    : _type(type)
    , _transport(transport)
    , _config(config)
    , _sinks(std::move(sinks))
    , _state(std::make_unique<State>())
{}

GPSDriver::~GPSDriver() = default;

bool GPSDriver::supportsType(GPSType type)
{
    return findProtocolFactory(type) != PROTOCOL_FACTORIES.end();
}

const std::vector<GPSConfigurationEvidence>& GPSDriver::configurationEvidence() const
{
    return _state->evidence;
}

const QString& GPSDriver::configurationError() const
{
    return _state->configurationError;
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
        _state->activity |= result.status == GPSReadStatus::Data && result.bytesRead > 0;
        return result;
    };
    io.write = [this](std::span<const uint8_t> bytes, GPSDeadline deadline) {
        const int remaining = deadline.remainingMilliseconds(MonotonicClock::nowUs());
        // Do not submit another part of a multipart command after its absolute deadline.
        if (_transport.isCancelled() || remaining == 0) {
            return GPSWriteResult{_transport.isCancelled() ? GPSWriteStatus::Cancelled : GPSWriteStatus::TimedOut};
        }
        return _transport.writeConfiguration(bytes.data(), static_cast<int>(bytes.size()), deadline.toQDeadlineTimer());
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
    io.log = [](GPSProtocolLogLevel level, QStringView message) {
        if (level == GPSProtocolLogLevel::Debug) {
            qCDebug(GPSNativeDriversLog) << message;
        } else {
            qCWarning(GPSNativeDriversLog) << message;
        }
    };
    io.commandFinished = [this](const GPSCommandResult& result) {
        if (_state->configuring) {
            _state->evidence.push_back(result.evidence);
        }
    };
    io.decoded = [this](const GPSDecodedBatch& batch) {
        _state->activity |= !batch.events.empty();
        for (const auto& event : batch.events) {
            std::visit(
                [this](const auto& report) {
                    using Report = std::decay_t<decltype(report)>;
                    if constexpr (std::is_same_v<Report, GPSNativeIntegrityReport>) {
                        _state->integrity = report;
                    } else if constexpr (std::is_same_v<Report, GPSNativePositionReport>) {
                        if (!_state->configuring) {
                            _state->usefulData = true;
                            _state->updates |= 1;
                            if (_sinks.onPosition) {
                                _sinks.onPosition(
                                    GPSNativeData::position(report, _state->integrity, MonotonicClock::nowUs()));
                            }
                        }
                    } else if constexpr (std::is_same_v<Report, GPSNativeSatelliteReport>) {
                        if (!_state->configuring) {
                            _state->usefulData = true;
                            _state->updates |= 2;
                            const auto snapshot = _state->satelliteSnapshot.update(report, MonotonicClock::nowUs());
                            if (_sinks.onSatelliteInfo) {
                                _sinks.onSatelliteInfo(snapshot);
                            }
                        }
                    } else if constexpr (std::is_same_v<Report, GPSSatelliteUsageReport>) {
                        if (!_state->configuring) {
                            _state->usefulData = true;
                            _state->updates |= 2;
                            if (_sinks.onSatelliteUsage) {
                                _sinks.onSatelliteUsage(report);
                            }
                        }
                    } else if constexpr (std::is_same_v<Report, GPSNativeSurveyReport>) {
                        _state->usefulData = true;
                        if (_sinks.onSurveyIn) {
                            _sinks.onSurveyIn(GPSNativeData::survey(report));
                        }
                    } else if constexpr (std::is_same_v<Report, GPSRTCMReport>) {
                        _state->usefulData = true;
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
    _state->driver = factory->create(std::move(io), &_state->position, &_state->satellites);
    if (_type == GPSType::trimble && !_config.baudRate) {
        baudrate = 115200;
    }
    GPSProtocol::GPSConfig config{};
    config.base = _config.base;
    const bool asciiReceiver = _type == GPSType::unicore || _type == GPSType::quectel || _type == GPSType::passive;
    config.dynamicModel = static_cast<uint8_t>(_config.dynamicModel.value_or(asciiReceiver ? 0 : 7));
    config.output_mode =
        _config.role == GPSReceiverConfig::Role::RTKBase ? GPSProtocol::OutputMode::RTCM : GPSProtocol::OutputMode::GPS;
    config.gnss_systems = static_cast<GPSProtocol::GNSSSystemsMask>(_config.constellationMask);
    config.allowPersistentChanges = _config.allowPersistentChanges;
    _state->configuring = true;
    const int result = _state->driver->configure(baudrate, config);
    _state->driver->finishConfigurationEvidence();
    if (result >= 0) {
        // Configuration can finish by publishing a fixed-base or survey-start event without another read.
        _state->driver->consume({});
    }
    _state->configuring = false;
    if (result < 0) {
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
        return {GPSReceiveStatus::Busy, 0, -EBUSY, detail};
    }
    const QScopedValueRollback operation(_operationInProgress, true);
    if (!_state->driver) {
        return {GPSReceiveStatus::NotConfigured, 0, -1};
    }
    _state->updates = 0;
    _state->usefulData = false;
    _state->activity = false;
    _publishExpiredSatellites();
    const int result = _state->driver->receive(timeoutMs);
    _publishExpiredSatellites();
    const int error = _state->driver->ioError() ? _state->driver->ioError() : (result < -1 ? result : 0);
    if (error) {
        return {error == -ECANCELED ? GPSReceiveStatus::Cancelled
                : error == -EPROTO  ? GPSReceiveStatus::ProtocolError
                                    : GPSReceiveStatus::TransportError,
                _state->updates, error, _state->driver->ioErrorDetail()};
    }
    if (_transport.isCancelled()) {
        return {GPSReceiveStatus::Cancelled, _state->updates, -ECANCELED};
    }
    if (_transport.fatalError()) {
        return {GPSReceiveStatus::TransportError, _state->updates, -EIO};
    }
    return {_state->usefulData               ? GPSReceiveStatus::Data
            : _state->activity || result > 0 ? GPSReceiveStatus::Activity
                                             : GPSReceiveStatus::Idle,
            _state->updates, 0};
}

void GPSDriver::_publishExpiredSatellites()
{
    const auto expired = _state->satelliteSnapshot.expire(MonotonicClock::nowUs());
    if (expired && _sinks.onSatelliteInfo) {
        // Cache retirement is a notification, not new receiver traffic or navigation liveness.
        _sinks.onSatelliteInfo(*expired);
    }
}
