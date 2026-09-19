#include "GPSDriver.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <thread>
#include <type_traits>
#include <utility>

#include "GPSNativeData_p.h"
#include "GPSProtocol.h"
#include "GPSProtocolFeatures.h"
#include "GPSReceiverConfigValidation.h"
#include "GPSTransport.h"
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

namespace {
uint64_t nowUs()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
}  // namespace

struct GPSDriver::State
{
    GPSNativePositionReport position;
    GPSNativeSatelliteReport satellites;
    GPSNativeIntegrityReport integrity;
    std::unique_ptr<GPSProtocol> driver;
    std::vector<GPSConfigurationEvidence> evidence;
    bool configuring = false;
    int updates = 0;
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

bool GPSDriver::configure()
{
    _state = std::make_unique<State>();
    if (const QString error = gpsReceiverConfigError(_type, _config); !error.isEmpty()) {
        qCWarning(GPSDriverLog) << error;
        return false;
    }

    GPSProtocolIO io;
    io.nowUs = nowUs;
    io.read = [this](std::span<uint8_t> bytes, GPSDeadline deadline) {
        const auto now = nowUs();
        const int timeout = now >= deadline.untilUs
                                ? 0
                                : static_cast<int>(std::min<uint64_t>((deadline.untilUs - now + 999) / 1000, INT_MAX));
        const auto result = _transport.read(bytes.data(), static_cast<int>(bytes.size()), timeout);
        return GPSProtocolReadResult{static_cast<GPSNativeReadStatus>(result.status), result.bytesRead};
    };
    io.write = [this](std::span<const uint8_t> bytes, GPSDeadline) {
        // Android serial implements the synchronous configuration writer, not writeBounded().
        const auto result = _transport.write(bytes.data(), static_cast<int>(bytes.size()));
        return GPSProtocolWriteResult{static_cast<GPSNativeWriteStatus>(result.status), result.acceptedBytes,
                                      result.writtenBytes, result.uncertainBytes()};
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
    io.log = [](GPSProtocolLogLevel level, std::string_view message) {
        const auto text = QString::fromUtf8(message.data(), static_cast<qsizetype>(message.size()));
        if (level == GPSProtocolLogLevel::Debug) {
            qCDebug(GPSNativeDriversLog) << text;
        } else {
            qCWarning(GPSNativeDriversLog) << text;
        }
    };
    io.commandFinished = [this](const GPSCommandResult& result) {
        if (_state->configuring) {
            _state->evidence.push_back({result.command, result.outcome, result.startedAtUs, result.finishedAtUs,
                                        result.acceptedBytes, result.writtenBytes, result.uncertainBytes,
                                        result.required});
        }
    };
    io.decoded = [this](GPSDecodedBatch batch) {
        for (const auto& event : batch.events) {
            std::visit(
                [this](const auto& report) {
                    using Report = std::decay_t<decltype(report)>;
                    if constexpr (std::is_same_v<Report, GPSNativeIntegrityReport>) {
                        _state->integrity = report;
                    } else if constexpr (std::is_same_v<Report, GPSNativePositionReport>) {
                        if (!_state->configuring) {
                            _state->updates |= 1;
                            if (_sinks.onPosition) {
                                _sinks.onPosition(GPSNativeData::position(report, _state->integrity));
                            }
                        }
                    } else if constexpr (std::is_same_v<Report, GPSNativeSatelliteReport>) {
                        if (!_state->configuring) {
                            _state->updates |= 2;
                            if (_sinks.onSatelliteInfo) {
                                _sinks.onSatelliteInfo(GPSNativeData::satellites(report));
                            }
                        }
                    } else if constexpr (std::is_same_v<Report, GPSSatelliteUsageReport>) {
                        if (!_state->configuring && _type == GPSType::septentrio && report.usedCount) {
                            _state->updates |= 2;
                            if (_sinks.onSatelliteInfo) {
                                GPSSatelliteReport satellites;
                                satellites.timestampUs = report.timestamp;
                                satellites.count = static_cast<uint16_t>(
                                    std::clamp(*report.usedCount, 0, int(GPSSatelliteReport::MAX_SATELLITES)));
                                _sinks.onSatelliteInfo(satellites);
                            }
                        }
                    } else if constexpr (std::is_same_v<Report, GPSNativeSurveyReport>) {
                        if (_sinks.onSurveyIn) {
                            _sinks.onSurveyIn(GPSNativeData::survey(report));
                        }
                    } else if constexpr (std::is_same_v<Report, GPSRTCMReport>) {
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
        qCWarning(GPSDriverLog) << "Driver configuration failed for type" << static_cast<int>(_type);
        _state->driver.reset();
        return false;
    }
    return true;
}

int GPSDriver::receive(unsigned timeoutMs)
{
    if (!_state->driver) {
        return -1;
    }
    _state->updates = 0;
    const int result = _state->driver->receive(timeoutMs);
    return result < 0 ? result : _state->updates;
}
