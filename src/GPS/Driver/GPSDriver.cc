#include "GPSDriver.h"

#include <QtCore/QCoreApplication>

#include <ashtech.h>
#include <base_station.h>
#include <cstring>
#include <definitions.h>
#include <femtomes.h>
#include <gps_helper.h>
#include <numbers>
#include <sbf.h>
#include <ubx.h>
#include <utility>

#include "GPSDriverData.h"
#include "GPSTransport.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSDriverLog, "GPS.Driver.GPSDriver")
QGC_LOGGING_CATEGORY(GPSDriversLog, "GPS.Driver.Drivers")

struct GPSDriver::Private
{
    std::unique_ptr<GPSBaseStationSupport> driver;
    sensor_gps_s sensorGps{};
    satellite_info_s satelliteInfo{};
};

namespace {
int callbackTrampoline(GPSCallbackType type, void *data1, int data2, void *user)
{
    return static_cast<GPSDriver *>(user)->handleCallback(static_cast<int>(type), data1, data2);
}
} // namespace

GPSDriver::GPSDriver(GPSType type, GPSTransport& transport, const GPSReceiverConfig& config, GPSDriverSinks sinks)
    : _type(type)
    , _transport(transport)
    , _config(config)
    , _sinks(std::move(sinks))
    , _capabilities(GPSReceiverCapabilities::forType(type))
    , _private(std::make_unique<Private>())
{
    qCDebug(GPSDriverLog) << this;
    GPSDriverData::initialize(_private->sensorGps);
}

GPSDriver::~GPSDriver()
{
    qCDebug(GPSDriverLog) << this;
}

bool GPSDriver::configure()
{
    const auto tr = [](const char* text) { return QCoreApplication::translate("GPSDriver", text); };
    _baudrate = 0;
    _private->driver.reset();
    _capabilities = GPSReceiverCapabilities::forType(_type);
    _configurationResult = {};
    const QString validationError = _capabilities.validationError(_config);
    if (!validationError.isEmpty()) {
        _configurationResult = {ConfigurationStatus::Unsupported, validationError};
        if (!_capabilities.recognized()) {
            qCWarning(GPSDriverLog) << "Unsupported GPS type:" << static_cast<int>(_type);
        } else if (_config.role != GPSReceiverConfig::Role::RTKBase &&
                   _config.role != GPSReceiverConfig::Role::Position) {
            qCWarning(GPSDriverLog) << "Unsupported receiver role:" << static_cast<int>(_config.role);
        } else {
            qCWarning(GPSDriverLog) << "Unsupported receiver output protocol";
        }
        return false;
    }
    const bool baseStation = _config.role == GPSReceiverConfig::Role::RTKBase;
    unsigned baudrate = _transport.fixedBaudrate();
    switch (_type) {
        case GPSType::trimble:
            _private->driver.reset(
                new GPSDriverAshtech(&callbackTrampoline, this, &_private->sensorGps, &_private->satelliteInfo));
            baudrate = 115200;
            break;
        case GPSType::septentrio:
            _private->driver.reset(new GPSDriverSBF(&callbackTrampoline, this, &_private->sensorGps,
                                                    &_private->satelliteInfo,
                                                    _config.headingOffsetDeg * std::numbers::pi_v<float> / 180.0f));
            break;
        case GPSType::u_blox: {
            const GPSDriverUBX::Settings settings{
                .dynamic_model = 0,
                .dgnss_timeout = 0,
                .min_cno = 0,
                .min_elev = 0,
                .output_rate = 0,
                .heading_offset = 0.0f,
                .uart1_baudrate = 0,
                .uart2_baudrate = 57600,
                .ppk_output = false,
                .jam_det_sensitivity_hi = false,
                .mode = GPSDriverUBX::UBXMode::Normal,
            };
            _private->driver.reset(new GPSDriverUBX(GPSDriverUBX::Interface::UART, &callbackTrampoline, this,
                                                    &_private->sensorGps, &_private->satelliteInfo, settings));
            break;
        }
        case GPSType::femto:
            _private->driver.reset(
                new GPSDriverFemto(&callbackTrampoline, this, &_private->sensorGps, &_private->satelliteInfo));
            break;
    }

    if (!_private->driver) {
        _configurationResult = {ConfigurationStatus::Unsupported, tr("Unsupported receiver type")};
        qCWarning(GPSDriverLog) << "Unsupported GPS type:" << static_cast<int>(_type);
        return false;
    }

    if (baseStation) {
        if (_config.base.useFixedBase) {
            _private->driver->setBasePosition(_config.base.fixedBaseLatitude, _config.base.fixedBaseLongitude,
                                              _config.base.fixedBaseAltitudeMeters,
                                              _config.base.fixedBaseAccuracyMeters * 1000.0f);
        } else {
            _private->driver->setSurveyInSpecs(static_cast<uint32_t>(_config.base.surveyInAccMeters * 10000.0),
                                               static_cast<uint32_t>(_config.base.surveyInDurationSecs));
        }
    }

    GPSHelper::GPSConfig gpsConfig{};
    gpsConfig.output_mode = baseStation ? GPSHelper::OutputMode::RTCM : GPSHelper::OutputMode::GPS;
    int result;
    if (_type == GPSType::u_blox) {
        const auto protocol = _config.outputProtocol == GPSReceiverConfig::OutputProtocol::NMEA
                                  ? GPSDriverUBX::OutputProtocol::NMEA
                                  : GPSDriverUBX::OutputProtocol::Native;
        result = static_cast<GPSDriverUBX*>(_private->driver.get())->configure(baudrate, gpsConfig, protocol);
    } else {
        result = _private->driver->configure(baudrate, gpsConfig);
    }

    _updateCapabilities();
    const QString capabilityError = _capabilities.validationError(_config);
    if (result != 0 || !capabilityError.isEmpty() || _transport.isCancelled()) {
        if (_transport.isCancelled()) {
            _configurationResult = {ConfigurationStatus::Cancelled, {}};
        } else if (_transport.fatalError()) {
            _configurationResult = {ConfigurationStatus::TransportError, tr("Receiver transport failed")};
        } else if (!capabilityError.isEmpty()) {
            _configurationResult = {ConfigurationStatus::Unsupported, capabilityError};
        } else {
            _configurationResult = {ConfigurationStatus::Failed, tr("Receiver configuration failed")};
        }
        if (!_transport.isCancelled()) {
            qCWarning(GPSDriverLog) << "Driver configuration failed for type" << static_cast<int>(_type);
        }
        _private->driver.reset();
        return false;
    }

    _baudrate = baudrate;
    _configurationResult.status = ConfigurationStatus::Ready;
    GPSDriverData::initialize(_private->sensorGps);
    return true;
}

void GPSDriver::_updateCapabilities()
{
    if (_type != GPSType::u_blox || !_private->driver) {
        return;
    }
    const auto* driver = static_cast<const GPSDriverUBX*>(_private->driver.get());
    _capabilities.model = QString::fromLatin1(driver->modelName());
    _capabilities.firmware = QString::fromLatin1(driver->firmwareVersion());
    using Support = GPSReceiverCapabilities::Support;
    switch (driver->baseStationCapability()) {
        case GPSDriverUBX::BaseStationCapability::Supported:
            _capabilities.rtkBase = Support::Supported;
            break;
        case GPSDriverUBX::BaseStationCapability::Unsupported:
            _capabilities.rtkBase = Support::Unsupported;
            break;
        case GPSDriverUBX::BaseStationCapability::Unknown:
            break;
    }
}

GPSDriver::ReceiveResult GPSDriver::receiveResult(unsigned timeoutMs)
{
    if (_transport.isCancelled()) {
        return {ReceiveStatus::Cancelled};
    }
    if (!_private->driver) {
        return {};
    }
    const int result = receive(timeoutMs);
    if (_transport.isCancelled()) {
        return {ReceiveStatus::Cancelled};
    }
    if (_transport.fatalError()) {
        return {ReceiveStatus::DeviceError};
    }
    // Native drivers also return negative values for a receive timeout. Only the
    // transport can distinguish a fatal device error from this normal idle case.
    if (result <= 0) {
        return {ReceiveStatus::Idle};
    }
    return {ReceiveStatus::Data, (result & 0x01) != 0, (result & 0x02) != 0};
}

int GPSDriver::receive(unsigned timeoutMs)
{
    if (!_private->driver) {
        return -1;
    }

    const int ret = _private->driver->receive(timeoutMs);
    if (ret < 0) {
        return ret;
    }

    if ((ret & 0x01) && _sinks.onPosition) {
        _sinks.onPosition(GPSDriverData::position(_private->sensorGps));
    }
    if ((ret & 0x02) && _sinks.onSatelliteInfo) {
        _sinks.onSatelliteInfo(GPSDriverData::satellites(_private->satelliteInfo, _type));
    }
    return ret;
}

int GPSDriver::handleCallback(int type, void *data1, int data2)
{
    switch (static_cast<GPSCallbackType>(type)) {
    case GPSCallbackType::readDeviceData: {
        if (_transport.isCancelled()) {
            return GPSHelper::ReadCancelled;
        }
        int timeoutMs = 0;
        memcpy(&timeoutMs, data1, sizeof(timeoutMs)); // px4 packs the timeout into data1's first bytes (unaligned)
        const int result = _transport.read(static_cast<uint8_t*>(data1), data2, timeoutMs);
        return _transport.isCancelled() ? GPSHelper::ReadCancelled : result;
    }
    case GPSCallbackType::writeDeviceData:
        return _transport.write(static_cast<const uint8_t *>(data1), data2);
    case GPSCallbackType::setBaudrate:
        return _transport.setBaudrate(static_cast<unsigned>(data2)) ? 0 : -1;
    case GPSCallbackType::gotRTCMMessage:
        if (_config.role == GPSReceiverConfig::Role::RTKBase && _sinks.onRTCM) {
            _sinks.onRTCM(QByteArray(static_cast<const char *>(data1), data2));
        }
        break;
    case GPSCallbackType::gotRelativePositionMessage:
        if (data1 && data2 == sizeof(sensor_gnss_relative_s) && _sinks.onRelativePosition) {
            _sinks.onRelativePosition(
                GPSDriverData::relativePosition(*static_cast<const sensor_gnss_relative_s*>(data1)));
        }
        break;
    case GPSCallbackType::surveyInStatus:
        if (_config.role == GPSReceiverConfig::Role::RTKBase && data1 && _sinks.onSurveyIn) {
            const SurveyInStatus *const status = static_cast<const SurveyInStatus *>(data1);
            GPSSurveyInStatus out;
            out.latitude = status->latitude;
            out.longitude = status->longitude;
            out.altitude = status->altitude;
            out.meanAccuracyMM = status->mean_accuracy;
            out.durationSecs = status->duration;
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
