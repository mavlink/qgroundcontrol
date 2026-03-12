#include "GPSProvider.h"
#include "GPSTransport.h"
#include "QGCLoggingCategory.h"

#include <ashtech.h>
#include <base_station.h>
#include <definitions.h>
#include <femtomes.h>
#include <sbf.h>
#include <nmea.h>
#include <ubx.h>

#include <QtCore/QThread>

QGC_LOGGING_CATEGORY(GPSProviderLog, "GPS.GPSProvider")
QGC_LOGGING_CATEGORY(GPSDriversLog, "GPS.Drivers")

GPSProvider::GPSProvider(GPSTransport *transport, GPSType type, const rtk_data_s &rtkData, const std::atomic_bool &requestStop, QObject *parent)
    : QObject(parent)
    , _transport(transport)
    , _type(type)
    , _requestStop(requestStop)
    , _rtkData(rtkData)
{
    qCDebug(GPSProviderLog) << this;

    qCDebug(GPSProviderLog) << QStringLiteral("Survey in accuracy: %1 | duration: %2").arg(_rtkData.surveyInAccMeters).arg(_rtkData.surveyInDurationSecs);
}

GPSProvider::rtk_data_s GPSProvider::rtkDataFromSettings(RTKSettings &settings)
{
    rtk_data_s d{};
    d.surveyInAccMeters      = settings.surveyInAccuracyLimit()->rawValue().toDouble();
    d.surveyInDurationSecs   = settings.surveyInMinObservationDuration()->rawValue().toInt();
    d.useFixedBaseLocation   = static_cast<BaseModeDefinition::Mode>(settings.useFixedBasePosition()->rawValue().toInt());
    d.fixedBaseLatitude      = settings.fixedBasePositionLatitude()->rawValue().toDouble();
    d.fixedBaseLongitude     = settings.fixedBasePositionLongitude()->rawValue().toDouble();
    d.fixedBaseAltitudeMeters = settings.fixedBasePositionAltitude()->rawValue().toFloat();
    d.fixedBaseAccuracyMeters = settings.fixedBasePositionAccuracy()->rawValue().toFloat();
    d.outputMode             = settings.gpsOutputMode()->rawValue().toUInt();
    d.ubxMode                = settings.ubxMode()->rawValue().toUInt();
    d.ubxDynamicModel        = settings.ubxDynamicModel()->rawValue().toUInt();
    d.ubxUart2Baudrate       = settings.ubxUart2Baudrate()->rawValue().toInt();
    d.ubxPpkOutput           = settings.ubxPpkOutput()->rawValue().toBool();
    d.ubxDgnssTimeout        = settings.ubxDgnssTimeout()->rawValue().toUInt();
    d.ubxMinCno              = settings.ubxMinCno()->rawValue().toUInt();
    d.ubxMinElevation        = settings.ubxMinElevation()->rawValue().toUInt();
    d.ubxOutputRate          = settings.ubxOutputRate()->rawValue().toUInt();
    d.ubxJamDetSensitivityHi = settings.ubxJamDetSensitivityHi()->rawValue().toBool();
    return d;
}

GPSProvider::~GPSProvider()
{
    qCDebug(GPSProviderLog) << this;
}


void GPSProvider::_publishSatelliteInfo()
{
    emit satelliteInfoUpdate(_satelliteInfo);
}

void GPSProvider::_publishSensorGNSSRelative()
{
    emit sensorGnssRelativeUpdate(_sensorGnssRelative);
}

void GPSProvider::_publishSensorGPS()
{
    emit sensorGpsUpdate(_sensorGps);
}

void GPSProvider::_gotRTCMData(const uint8_t *data, size_t len)
{
    const QByteArray message(reinterpret_cast<const char*>(data), len);
    emit RTCMDataUpdate(message);
}

void GPSProvider::injectRTCMData(const QByteArray &data)
{
    if (data.isEmpty()) {
        return;
    }

    QMutexLocker locker(&_rtcmMutex);
    _rtcmBuffer.append(data);
}

void GPSProvider::_drainRTCMBuffer()
{
    QByteArray data;
    {
        QMutexLocker locker(&_rtcmMutex);
        if (_rtcmBuffer.isEmpty()) {
            return;
        }
        data.swap(_rtcmBuffer);
    }

    if (_transport && _transport->isOpen() && _transport->write(data.constData(), data.size()) < 0) {
        qCWarning(GPSProviderLog) << "Failed to inject RTCM data:" << data.size() << "bytes";
    }
}

int GPSProvider::_callbackEntry(GPSCallbackType type, void *data1, int data2, void *user)
{
    GPSProvider* const gps = reinterpret_cast<GPSProvider*>(user);
    return gps->_callback(type, data1, data2);
}

int GPSProvider::_callback(GPSCallbackType type, void *data1, int data2)
{
    switch (type) {
    case GPSCallbackType::readDeviceData:
        if (!_transport || !_transport->isOpen()) {
            return -1;
        }

        _drainRTCMBuffer();

        if (_transport->bytesAvailable() == 0) {
            const int timeout = *(reinterpret_cast<int*>(data1));
            if (!_transport->waitForReadyRead(timeout)) {
                return 0;
            }
        }
        return _transport->read(reinterpret_cast<char*>(data1), data2);
    case GPSCallbackType::writeDeviceData:
        if (!_transport || !_transport->isOpen()) {
            return -1;
        }
        if (_transport->write(reinterpret_cast<char*>(data1), data2) >= 0) {
            if (_transport->waitForBytesWritten(kGPSReceiveTimeout)) {
                return data2;
            }
        }
        return -1;
    case GPSCallbackType::setBaudrate:
        return (_transport && _transport->setBaudRate(data2)) ? 0 : -1;
    case GPSCallbackType::gotRTCMMessage:
        _gotRTCMData(reinterpret_cast<uint8_t*>(data1), data2);
        break;
    case GPSCallbackType::surveyInStatus:
    {
        const SurveyInStatus* const raw = reinterpret_cast<SurveyInStatus*>(data1);

        SurveyInStatusData status;
        status.duration = raw->duration;
        status.accuracyMM = raw->mean_accuracy;
        status.latitude = raw->latitude;
        status.longitude = raw->longitude;
        status.altitude = raw->altitude;
        status.valid = raw->flags & 1;
        status.active = (raw->flags >> 1) & 1;

        qCDebug(GPSProviderLog) << QStringLiteral("Survey-in: %1s accuracy=%2mm valid=%3 active=%4")
            .arg(status.duration).arg(status.accuracyMM).arg(status.valid).arg(status.active);
        emit surveyInStatus(status);
        break;
    }
    case GPSCallbackType::gotRelativePositionMessage:
        _publishSensorGNSSRelative();
        break;
    case GPSCallbackType::setClock:
    default:
        break;
    }

    return 0;
}

void GPSProvider::process()
{
    if (!_transport || !_transport->open()) {
        qCWarning(GPSProviderLog) << "Transport open failed, exiting GPS thread";
        emit error(QStringLiteral("Failed to open GPS transport"));
        emit finished();
        return;
    }

    GPSHelper *gpsDriver = nullptr;

    while (!_requestStop && !QThread::currentThread()->isInterruptionRequested()) {
        if (gpsDriver) {
            delete gpsDriver;
            gpsDriver = nullptr;
            for (uint32_t elapsed = 0; elapsed < kReconnectDelayMs && !_requestStop; elapsed += 100)
                QThread::msleep(100);
            if (_requestStop) break;
        }

        gpsDriver = _connectGPS();
        if (gpsDriver) {
            emit connectionEstablished();
            (void) memset(&_sensorGps, 0, sizeof(_sensorGps));

            uint8_t numTries = 0;
            while (!_requestStop && !QThread::currentThread()->isInterruptionRequested() && (numTries < 3)) {
                _drainRTCMBuffer();

                const int helperRet = gpsDriver->receive(kGPSReceiveTimeout);

                if (helperRet > 0) {
                    numTries = 0;

                    if (helperRet & GPSReceiveType::Position) {
                        _publishSensorGPS();
                    }

                    if (helperRet & GPSReceiveType::Satellite) {
                        _publishSatelliteInfo();
                    }
                } else {
                    ++numTries;
                }
            }

            if (!_transport->isOpen()) {
                emit error(QStringLiteral("GPS transport closed unexpectedly"));
                break;
            }
        }
    }

    delete gpsDriver;
    gpsDriver = nullptr;

    _transport->close();

    qCDebug(GPSProviderLog) << Q_FUNC_INFO << "Exiting GPS thread";
    emit finished();
}


GPSHelper *GPSProvider::_connectGPS()
{
    GPSHelper *gpsDriver = nullptr;
    GPSBaseStationSupport *baseStationDriver = nullptr;
    uint32_t baudrate = 0;

    switch (_type) {
    case GPSType::Trimble:
        baseStationDriver = new GPSDriverAshtech(&_callbackEntry, this, &_sensorGps, &_satelliteInfo);
        gpsDriver = baseStationDriver;
        baudrate = 115200;
        break;
    case GPSType::Septentrio:
        baseStationDriver = new GPSDriverSBF(&_callbackEntry, this, &_sensorGps, &_satelliteInfo, kGPSHeadingOffset);
        gpsDriver = baseStationDriver;
        baudrate = 0;
        break;
    case GPSType::UBlox:
    {
        GPSDriverUBX::Settings ubxSettings{};
        ubxSettings.dynamic_model = _rtkData.ubxDynamicModel;
        ubxSettings.heading_offset = kGPSHeadingOffset;
        ubxSettings.uart2_baudrate = _rtkData.ubxUart2Baudrate;
        ubxSettings.ppk_output = _rtkData.ubxPpkOutput;
        ubxSettings.mode = static_cast<GPSDriverUBX::UBXMode>(_rtkData.ubxMode);
        ubxSettings.dgnss_timeout = _rtkData.ubxDgnssTimeout;
        ubxSettings.min_cno = _rtkData.ubxMinCno;
        ubxSettings.min_elev = _rtkData.ubxMinElevation;
        ubxSettings.output_rate = _rtkData.ubxOutputRate;
        ubxSettings.jam_det_sensitivity_hi = _rtkData.ubxJamDetSensitivityHi;
        baseStationDriver = new GPSDriverUBX(GPSDriverUBX::Interface::UART, &_callbackEntry, this, &_sensorGps, &_satelliteInfo, ubxSettings);
        gpsDriver = baseStationDriver;
        baudrate = 0;
        break;
    }
    case GPSType::Femto:
        baseStationDriver = new GPSDriverFemto(&_callbackEntry, this, &_sensorGps, &_satelliteInfo);
        gpsDriver = baseStationDriver;
        baudrate = 0;
        break;
    case GPSType::NMEA:
        gpsDriver = new GPSDriverNMEA(&_callbackEntry, this, &_sensorGps, &_satelliteInfo, kGPSHeadingOffset);
        baudrate = NMEA_DEFAULT_BAUDRATE;
        break;
    default:
        qCWarning(GPSProviderLog) << "Unsupported GPS Type:" << static_cast<int>(_type);
        return nullptr;
    }

    if (baseStationDriver) {
        switch (_rtkData.useFixedBaseLocation) {
        case BaseModeDefinition::Mode::BaseFixed:
            baseStationDriver->setBasePosition(_rtkData.fixedBaseLatitude, _rtkData.fixedBaseLongitude, _rtkData.fixedBaseAltitudeMeters, _rtkData.fixedBaseAccuracyMeters * 1000.0f);
            break;
        case BaseModeDefinition::Mode::BaseSurveyIn:
        default:
            baseStationDriver->setSurveyInSpecs(_rtkData.surveyInAccMeters * 10000.f, _rtkData.surveyInDurationSecs);
            break;
        }
        _gpsConfig.output_mode = static_cast<GPSHelper::OutputMode>(_rtkData.outputMode);
    } else {
        _gpsConfig.output_mode = GPSHelper::OutputMode::GPS;
    }

    if (gpsDriver->configure(baudrate, _gpsConfig) != 0) {
        delete gpsDriver;
        return nullptr;
    }

    return gpsDriver;
}
