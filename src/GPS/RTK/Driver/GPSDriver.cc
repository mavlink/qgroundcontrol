#include "GPSDriver.h"

#include <ashtech.h>
#include <base_station.h>
#include <cstring>
#include <definitions.h>
#include <femtomes.h>
#include <gps_helper.h>
#include <limits>
#include <sbf.h>
#include <ubx.h>
#include <utility>

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

GPSDriver::GPSDriver(GPSReceiverType type, GPSTransport& transport, const GPSReceiverConfig& config,
                     GPSDriverSinks sinks)
    : _type(type), _transport(transport), _config(config), _sinks(std::move(sinks))
{}

GPSDriver::~GPSDriver() = default;

bool GPSDriver::configure()
{
    _driver.reset();
    const auto* fixed = std::get_if<GPSFixedBaseConfig>(&_config.base);
    const auto* survey = std::get_if<GPSSurveyInConfig>(&_config.base);
    constexpr double MAX_UNSIGNED_VALUE = (std::numeric_limits<uint32_t>::max)();
    if (fixed) {
        if (!fixed->coordinate.isValid() || !qIsFinite(fixed->altitudeEllipsoidMeters)) {
            qCWarning(GPSDriverLog) << "Fixed base position requires valid coordinates and finite ellipsoid altitude";
            return false;
        }
        const double altitudeCm = static_cast<double>(fixed->altitudeEllipsoidMeters) * 100.0;
        // PX4 passes accuracy as float millimeters, then UBX converts to unsigned 0.1 mm.
        const double accuracyUnits = static_cast<double>((fixed->accuracyMeters * 1000.0f) * 10.0f);
        if (altitudeCm < (std::numeric_limits<int32_t>::min)() || altitudeCm > (std::numeric_limits<int32_t>::max)() ||
            !qIsFinite(accuracyUnits) || accuracyUnits < 0 || accuracyUnits > MAX_UNSIGNED_VALUE) {
            qCWarning(GPSDriverLog) << "Fixed base altitude or accuracy exceeds driver limits";
            return false;
        }
    } else if (survey) {
        const double accuracyUnits = survey->accuracyMeters * 10000.0;
        const auto duration = survey->minimumDuration.count();
        if (!qIsFinite(accuracyUnits) || accuracyUnits < 1 || accuracyUnits > MAX_UNSIGNED_VALUE || duration < 1 ||
            duration > (std::numeric_limits<uint32_t>::max)()) {
            qCWarning(GPSDriverLog) << "Survey-in requires representable positive accuracy and duration";
            return false;
        }
    } else {
        qCWarning(GPSDriverLog) << "Missing base configuration";
        return false;
    }

    unsigned baudrate = _transport.fixedBaudrate();
    switch (_type) {
        case GPSReceiverType::trimble:
            _driver.reset(new GPSDriverAshtech(&callbackTrampoline, this, &_sensorGps, &_satelliteInfo));
            baudrate = 115200;
            break;
        case GPSReceiverType::septentrio:
            _driver.reset(new GPSDriverSBF(&callbackTrampoline, this, &_sensorGps, &_satelliteInfo));
            break;
        case GPSReceiverType::ublox: {
            const GPSDriverUBX::Settings settings{
                .dynamic_model = 7,
                .dgnss_timeout = 0,
                .min_cno = 0,
                .min_elev = 0,
                .output_rate = 0,
                .heading_offset = 0.0f,
                .uart2_baudrate = 57600,
                .ppk_output = false,
                .jam_det_sensitivity_hi = false,
                .mode = GPSDriverUBX::UBXMode::Normal,
            };
            _driver.reset(new GPSDriverUBX(GPSDriverUBX::Interface::UART, &callbackTrampoline, this, &_sensorGps,
                                           &_satelliteInfo, settings));
            break;
        }
        case GPSReceiverType::femto:
            _driver.reset(new GPSDriverFemto(&callbackTrampoline, this, &_sensorGps, &_satelliteInfo));
            break;
    }

    if (!_driver) {
        qCWarning(GPSDriverLog) << "Unsupported GPS type:" << static_cast<int>(_type);
        return false;
    }

    if (fixed) {
        _driver->setBasePosition(fixed->coordinate.latitude(), fixed->coordinate.longitude(),
                                 fixed->altitudeEllipsoidMeters, fixed->accuracyMeters * 1000.0f);
    } else {
        _driver->setSurveyInSpecs(static_cast<uint32_t>(survey->accuracyMeters * 10000.0),
                                  static_cast<uint32_t>(survey->minimumDuration.count()));
    }

    GPSHelper::GPSConfig gpsConfig{};
    gpsConfig.output_mode = GPSHelper::OutputMode::RTCM;

    if (_driver->configure(baudrate, gpsConfig) != 0) {
        qCWarning(GPSDriverLog) << "Driver configuration failed for type" << static_cast<int>(_type);
        _driver.reset();
        return false;
    }

    (void) memset(&_sensorGps, 0, sizeof(_sensorGps));
    return true;
}

int GPSDriver::receive(unsigned timeoutMs)
{
    if (!_driver) {
        return -1;
    }

    const int ret = _driver->receive(timeoutMs);
    if (ret < 0) {
        return ret;
    }

    if ((ret & 0x01) && _sinks.onPosition) {
        _sinks.onPosition(_sensorGps);
    }
    if ((ret & 0x02) && _sinks.onSatelliteInfo) {
        _sinks.onSatelliteInfo(_satelliteInfo);
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
            if (result.status == GPSTransport::ReadStatus::Data && result.bytesRead >= 0 && result.bytesRead <= data2) {
                return result.bytesRead;
            }
            return result.status == GPSTransport::ReadStatus::TimedOut ? 0 : -1;
        }
        case GPSCallbackType::writeDeviceData: {
            const auto result = _transport.write(static_cast<const uint8_t*>(data1), data2);
            return result.status == GPSTransport::WriteStatus::Completed && result.acceptedBytes == data2 &&
                           result.writtenBytes == data2 && result.uncertainBytes == 0
                       ? data2
                       : -1;
        }
        case GPSCallbackType::setBaudrate:
            return _transport.setBaudrate(static_cast<unsigned>(data2)) ? 0 : -1;
        case GPSCallbackType::gotRTCMMessage:
            if (_sinks.onRTCM) {
                _sinks.onRTCM(QByteArray(static_cast<const char*>(data1), data2));
            }
            break;
        case GPSCallbackType::surveyInStatus:
            if (data1 && _sinks.onSurveyIn) {
                const SurveyInStatus* const status = static_cast<const SurveyInStatus*>(data1);
                GPSSurveyInStatus out;
                out.coordinate = QGeoCoordinate(status->latitude, status->longitude);
                out.altitudeEllipsoidMeters = status->altitude;
                // Ashtech and Femto use zero for unknown accuracy; UBX can round a valid value to zero.
                if (status->mean_accuracy != 0 ||
                    (_type != GPSReceiverType::trimble && _type != GPSReceiverType::femto)) {
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
