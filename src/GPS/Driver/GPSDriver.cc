#include "GPSDriver.h"

#include <ashtech.h>
#include <base_station.h>
#include <cstring>
#include <definitions.h>
#include <femtomes.h>
#include <gps_helper.h>
#include <sbf.h>
#include <ubx.h>
#include <utility>

#include "GPSPx4Data_p.h"
#include "GPSReceiverConfigValidation.h"
#include "GPSTransport.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSDriverLog, "GPS.GPSDriver")
QGC_LOGGING_CATEGORY(GPSDriversLog, "GPS.Drivers")  // backs the px4 GPS_INFO/WARN/ERR macros in definitions.h

namespace {
int callbackTrampoline(GPSCallbackType type, void* data1, int data2, void* user)
{
    return static_cast<GPSDriver*>(user)->handleCallback(static_cast<int>(type), data1, data2);
}
}  // namespace

struct GPSDriver::State
{
    State() { GPSPx4Data::initialize(position); }

    sensor_gps_s position{};
    satellite_info_s satellites{};
    std::unique_ptr<GPSBaseStationSupport> driver;
};

GPSDriver::GPSDriver(GPSType type, GPSTransport& transport, const GPSReceiverConfig& config, GPSDriverSinks sinks)
    : _type(type)
    , _transport(transport)
    , _config(config)
    , _sinks(std::move(sinks))
    , _state(std::make_unique<State>())
{}

GPSDriver::~GPSDriver() = default;

bool GPSDriver::configure()
{
    _state->driver.reset();
    GPSPx4Data::initialize(_state->position);
    _state->satellites = {};
    if (const QString error = gpsReceiverConfigError(_type, _config); !error.isEmpty()) {
        qCWarning(GPSDriverLog) << error;
        return false;
    }

    unsigned baudrate = _transport.fixedBaudrate();
    const float headingOffset = _config.headingOffsetRadians.value_or(0.0f);
    switch (_type) {
        case GPSType::trimble:
            _state->driver = std::make_unique<GPSDriverAshtech>(&callbackTrampoline, this, &_state->position,
                                                                &_state->satellites, headingOffset);
            baudrate = 115200;
            break;
        case GPSType::septentrio:
            _state->driver = std::make_unique<GPSDriverSBF>(&callbackTrampoline, this, &_state->position,
                                                            &_state->satellites, headingOffset);
            break;
        case GPSType::ublox: {
            const GPSDriverUBX::Settings settings{
                .dynamic_model = static_cast<uint8_t>(_config.dynamicModel.value_or(7)),
                .dgnss_timeout = 0,
                .min_cno = 0,
                .min_elev = 0,
                .output_rate = 0,
                .heading_offset = headingOffset,
                .uart2_baudrate = 57600,
                .ppk_output = false,
                .jam_det_sensitivity_hi = false,
                .mode = GPSDriverUBX::UBXMode::Normal,
            };
            _state->driver = std::make_unique<GPSDriverUBX>(GPSDriverUBX::Interface::UART, &callbackTrampoline, this,
                                                            &_state->position, &_state->satellites, settings);
            break;
        }
        case GPSType::femto:
            _state->driver = std::make_unique<GPSDriverFemto>(&callbackTrampoline, this, &_state->position,
                                                              &_state->satellites, headingOffset);
            break;
    }

    if (!_state->driver) {
        qCWarning(GPSDriverLog) << "Unsupported GPS type:" << static_cast<int>(_type);
        return false;
    }

    if (_config.role == GPSReceiverConfig::Role::RTKBase) {
        const auto& base = _config.base;
        if (base.useFixedBase) {
            _state->driver->setBasePosition(base.fixedBaseLatitude, base.fixedBaseLongitude,
                                            base.fixedBaseAltitudeMeters, base.fixedBaseAccuracyMeters * 1000.0f);
        } else {
            _state->driver->setSurveyInSpecs(static_cast<uint32_t>(base.surveyInAccMeters * 10000.0),
                                             static_cast<uint32_t>(base.surveyInDurationSecs));
        }
    }

    GPSHelper::GPSConfig gpsConfig{};
    gpsConfig.output_mode =
        _config.role == GPSReceiverConfig::Role::RTKBase ? GPSHelper::OutputMode::RTCM : GPSHelper::OutputMode::GPS;
    gpsConfig.gnss_systems = static_cast<GPSHelper::GNSSSystemsMask>(_config.constellationMask);

    if (_state->driver->configure(baudrate, gpsConfig) != 0) {
        qCWarning(GPSDriverLog) << "Driver configuration failed for type" << static_cast<int>(_type);
        _state->driver.reset();
        return false;
    }

    GPSPx4Data::initialize(_state->position);
    return true;
}

int GPSDriver::receive(unsigned timeoutMs)
{
    if (!_state->driver) {
        return -1;
    }

    const int ret = _state->driver->receive(timeoutMs);
    if (ret < 0) {
        return ret;
    }

    if ((ret & 0x01) && _sinks.onPosition) {
        _sinks.onPosition(GPSPx4Data::position(_state->position));
    }
    if ((ret & 0x02) && _sinks.onSatelliteInfo) {
        _sinks.onSatelliteInfo(GPSPx4Data::satellites(_state->satellites, _type));
    }
    return ret;
}

int GPSDriver::handleCallback(int type, void* data1, int data2)
{
    switch (static_cast<GPSCallbackType>(type)) {
        case GPSCallbackType::readDeviceData: {
            int timeoutMs = 0;
            memcpy(&timeoutMs, data1, sizeof(timeoutMs));  // px4 packs the timeout into data1's first bytes (unaligned)
            const auto result = _transport.read(static_cast<uint8_t*>(data1), data2, timeoutMs);
            if (result.status == GPSReadStatus::Data && result.bytesRead >= 0 && result.bytesRead <= data2) {
                return result.bytesRead;
            }
            return result.status == GPSReadStatus::TimedOut ? 0 : -1;
        }
        case GPSCallbackType::writeDeviceData: {
            const auto result = _transport.write(static_cast<const uint8_t*>(data1), data2);
            return result.status == GPSWriteStatus::Completed && result.acceptedBytes == data2 &&
                           result.writtenBytes == data2 && result.uncertainBytes() == 0
                       ? data2
                       : -1;
        }
        case GPSCallbackType::setBaudrate:
            return _transport.setBaudrate(static_cast<unsigned>(data2)) ? 0 : -1;
        case GPSCallbackType::gotRTCMMessage:
            if (!data1 || data2 <= 0) {
                qCWarning(GPSDriverLog) << "Invalid RTCM callback payload";
                return -1;
            }
            if (_sinks.onRTCM) {
                _sinks.onRTCM({static_cast<const uint8_t*>(data1), static_cast<std::size_t>(data2)});
            }
            break;
        case GPSCallbackType::surveyInStatus:
            if (data1 && _sinks.onSurveyIn) {
                const SurveyInStatus* const status = static_cast<const SurveyInStatus*>(data1);
                GPSSurveyReport out;
                out.latitudeDegrees = status->latitude;
                out.longitudeDegrees = status->longitude;
                out.altitudeEllipsoidMeters = status->altitude;
                // Ashtech and Femto use zero for unknown accuracy; UBX can round a valid value to zero.
                if (status->mean_accuracy != 0 || (_type != GPSType::trimble && _type != GPSType::femto)) {
                    out.meanAccuracyMeters = static_cast<double>(status->mean_accuracy) / 1000.0;
                }
                out.duration = std::chrono::seconds(status->duration);
                out.valid = status->flags & 0x01;
                out.active = (status->flags >> 1) & 0x01;
                _sinks.onSurveyIn(out);
            }
            break;
        case GPSCallbackType::setClock:
        default:
            break;
    }

    return 0;
}
