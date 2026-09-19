#include "GPSDriver.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <thread>
#include <type_traits>
#include <utility>

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

QGC_LOGGING_CATEGORY(GPSDriverLog, "GPS.GPSDriver")
QGC_LOGGING_CATEGORY(GPSNativeDriversLog, "GPS.Drivers")

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
            _state->evidence.push_back({result.command, result.outcome, result.startedAtUs, result.finishedAtUs,
                                        result.acceptedBytes, result.writtenBytes, result.uncertainBytes,
                                        result.required});
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
                                _sinks.onPosition(GPSNativeData::position(report, _state->integrity));
                            }
                        }
                    } else if constexpr (std::is_same_v<Report, GPSNativeSatelliteReport>) {
                        if (!_state->configuring) {
                            _state->usefulData = true;
                            _state->updates |= 2;
                            const auto snapshot = _state->satelliteSnapshot.update(report);
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

    unsigned baudrate = _transport.fixedBaudrate();
    switch (_type) {
#if QGC_GPS_ENABLE_UBX
        case GPSType::ublox:
            _state->driver = std::make_unique<GPSNativeUBX>(io, &_state->position, &_state->satellites);
            break;
#endif
#if QGC_GPS_ENABLE_ASHTECH
        case GPSType::trimble:
            _state->driver = std::make_unique<GPSNativeAshtech>(io, &_state->position, &_state->satellites);
            baudrate = 115200;
            break;
#endif
#if QGC_GPS_ENABLE_SBF
        case GPSType::septentrio:
            _state->driver = std::make_unique<GPSNativeSBF>(io, &_state->position, &_state->satellites);
            break;
#endif
#if QGC_GPS_ENABLE_FEMTO
        case GPSType::femto:
            _state->driver = std::make_unique<GPSNativeFemto>(io, &_state->position, &_state->satellites);
            break;
#endif
        default:
            _state->configurationError = QStringLiteral("Unsupported GPS type: %1").arg(static_cast<int>(_type));
            qCWarning(GPSDriverLog) << "Unsupported GPS type:" << static_cast<int>(_type);
            return false;
    }
    GPSProtocol::GPSConfig config{};
    config.base = _config.base;
    config.dynamicModel = static_cast<uint8_t>(_config.dynamicModel.value_or(7));
    config.output_mode =
        _config.role == GPSReceiverConfig::Role::RTKBase ? GPSProtocol::OutputMode::RTCM : GPSProtocol::OutputMode::GPS;
    config.gnss_systems = static_cast<GPSProtocol::GNSSSystemsMask>(_config.constellationMask);
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
    return true;
}

GPSReceiveResult GPSDriver::receiveOutcome(unsigned timeoutMs)
{
    if (!_state->driver) {
        return {GPSReceiveStatus::NotConfigured, 0, -1};
    }
    _state->updates = 0;
    _state->usefulData = false;
    _state->activity = false;
    const int result = _state->driver->receive(timeoutMs);
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
