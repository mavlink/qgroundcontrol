#include "GPSProvider.h"
#include "GPSTransport.h"
#include "SerialGPSTransport.h"
#include "QGCLoggingCategory.h"

#include <ashtech.h>
#include <base_station.h>
#include <definitions.h>
#include <emlid_reach.h>
#include <femtomes.h>
#include <mtk.h>
#include <sbf.h>
#include <nmea.h>
#include <ubx.h>

#include <QtCore/QThread>
#include <memory>

QGC_LOGGING_CATEGORY(GPSProviderLog, "GPS.GPSProvider")
QGC_LOGGING_CATEGORY(GPSDriversLog, "GPS.Drivers")
// Defined here rather than in its own translation unit so GPSTypes.h's inline
// helpers (gpsTypeFromString / gpsTypeDisplayName) always resolve against this
// category without an extra .cc just to host one Q_LOGGING_CATEGORY macro.
QGC_LOGGING_CATEGORY(GPSTypesLog, "GPS.GPSTypes")

GPSProvider::GPSProvider(GPSTransport *transport, GPSType type, const rtk_data_s &rtkData, QObject *parent)
    : QObject(parent)
    , _transport(transport)
    , _type(type)
    , _rtkData(rtkData)
{
    qCDebug(GPSProviderLog) << this;

    qCDebug(GPSProviderLog) << "Survey in accuracy:" << _rtkData.surveyInAccMeters
        << "| duration:" << _rtkData.surveyInDurationSecs;
}

GPSProvider::~GPSProvider()
{
    qCDebug(GPSProviderLog) << this;
}

void GPSProvider::wakeFromReconnectWait()
{
    QMutexLocker locker(&_stopMutex);
    _stopCondition.wakeAll();

    // Also cancel any serial port retry sleep
    if (auto *serial = qobject_cast<SerialGPSTransport*>(_transport)) {
        serial->cancelRetry();
    }
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
    case GPSCallbackType::readDeviceData: {
        // PX4 convention: timeout is smuggled in first 4 bytes of data1 buffer
        // data1 = buffer (with timeout in first bytes), data2 = buffer length
        if (!_transport || !_transport->isOpen()) {
            return -1;
        }

        _drainRTCMBuffer();

        int timeout = 0;
        if (data2 < static_cast<int>(sizeof(timeout))) {
            return -1;
        }
        memcpy(&timeout, data1, sizeof(timeout));

        if (_transport->bytesAvailable() == 0) {
            if (!_transport->waitForReadyRead(timeout)) {
                return 0;
            }
        }
        return static_cast<int>(_transport->read(reinterpret_cast<char*>(data1), data2));
    }
    case GPSCallbackType::writeDeviceData:
        if (!_transport || !_transport->isOpen()) {
            return -1;
        }
        if (_transport->write(reinterpret_cast<char*>(data1), data2) >= 0) {
            if (_transport->waitForBytesWritten(kReceiveTimeoutMs)) {
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

        qCDebug(GPSProviderLog) << "Survey-in:" << status.duration << "s accuracy="
            << status.accuracyMM << "mm valid=" << status.valid << "active=" << status.active;
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
    qCDebug(GPSProviderLog) << "Transport opened";

    std::unique_ptr<GPSHelper> gpsDriver;

    while (!_requestStop && !QThread::currentThread()->isInterruptionRequested()) {
        if (gpsDriver) {
            QMutexLocker locker(&_stopMutex);
            gpsDriver.reset();
            if (_requestStop) break;
            _stopCondition.wait(&_stopMutex, kReconnectIntervalMs);
            if (_requestStop) break;
        }

        gpsDriver = _connectGPS();
        if (!gpsDriver) {
            qCWarning(GPSProviderLog) << "Driver configuration failed for type"
                                      << gpsTypeDisplayName(_type)
                                      << "(" << static_cast<int>(_type) << ")";
            emit error(QStringLiteral("GPS driver configuration failed"));
            QMutexLocker locker(&_stopMutex);
            if (_requestStop) break;
            _stopCondition.wait(&_stopMutex, kReconnectIntervalMs);
            continue;
        }
        {
            qCDebug(GPSProviderLog) << "Driver connected, type:" << gpsTypeDisplayName(_type);
            emit connectionEstablished();
            (void) memset(&_sensorGps, 0, sizeof(_sensorGps));

            uint8_t numTries = 0;
            static constexpr uint8_t kMaxReceiveTries = 5;
            while (!_requestStop && !QThread::currentThread()->isInterruptionRequested() && (numTries < kMaxReceiveTries)) {
                _drainRTCMBuffer();

                const int helperRet = gpsDriver->receive(kReceiveTimeoutMs);

                if (helperRet > 0) {
                    numTries = 0;

                    if (helperRet & GPSReceiveType::Position) {
                        _publishSensorGPS();
                    }

                    if (helperRet & GPSReceiveType::Satellite) {
                        _publishSatelliteInfo();
                    }
                } else {
                    // Some drivers (NMEA) return -1 on timeout even though they've
                    // partially updated _sensorGps (e.g. GGA parsed but no VTG/RMC for velocity).
                    // Publish position if the driver wrote valid coordinates.
                    if (_sensorGps.fix_type >= sensor_gps_s::FIX_TYPE_2D
                        && (_sensorGps.latitude_deg != 0. || _sensorGps.longitude_deg != 0.)) {
                        qCDebug(GPSProviderLog) << "Publishing partial position from driver: fix="
                                                 << _sensorGps.fix_type << "lat=" << _sensorGps.latitude_deg;
                        _publishSensorGPS();
                        numTries = 0;
                    } else {
                        ++numTries;
                        qCDebug(GPSProviderLog) << "receive() returned" << helperRet
                                                 << "fix=" << _sensorGps.fix_type << "try" << numTries;
                    }
                }
            }

            if (!_transport->isOpen()) {
                emit error(QStringLiteral("GPS transport closed unexpectedly"));
                break;
            }
        }
    }

    gpsDriver.reset();

    _transport->close();

    qCDebug(GPSProviderLog) << Q_FUNC_INFO << "Exiting GPS thread";
    emit finished();
}


std::unique_ptr<GPSHelper> GPSProvider::_connectGPS()
{
    std::unique_ptr<GPSHelper> gpsDriver;
    GPSBaseStationSupport *baseStationDriver = nullptr;
    uint32_t baudrate = 0;

    switch (_type) {
    case GPSType::Trimble:
        baseStationDriver = new GPSDriverAshtech(&_callbackEntry, this, &_sensorGps, &_satelliteInfo);
        gpsDriver.reset(baseStationDriver);
        baudrate = 115200;
        break;
    case GPSType::Septentrio:
        baseStationDriver = new GPSDriverSBF(&_callbackEntry, this, &_sensorGps, &_satelliteInfo, _rtkData.headingOffset);
        gpsDriver.reset(baseStationDriver);
        baudrate = 0;
        break;
    case GPSType::UBlox:
    {
        GPSDriverUBX::Settings ubxSettings{};
        ubxSettings.dynamic_model = _rtkData.ubxDynamicModel;
        ubxSettings.heading_offset = _rtkData.headingOffset;
        ubxSettings.uart2_baudrate = _rtkData.ubxUart2Baudrate;
        ubxSettings.ppk_output = _rtkData.ubxPpkOutput;
        ubxSettings.mode = static_cast<GPSDriverUBX::UBXMode>(_rtkData.ubxMode);
        ubxSettings.dgnss_timeout = _rtkData.ubxDgnssTimeout;
        ubxSettings.min_cno = _rtkData.ubxMinCno;
        ubxSettings.min_elev = _rtkData.ubxMinElevation;
        ubxSettings.output_rate = _rtkData.ubxOutputRate;
        ubxSettings.jam_det_sensitivity_hi = _rtkData.ubxJamDetSensitivityHi;
        baseStationDriver = new GPSDriverUBX(GPSDriverUBX::Interface::UART, &_callbackEntry, this, &_sensorGps, &_satelliteInfo, ubxSettings);
        gpsDriver.reset(baseStationDriver);
        baudrate = 0;
        break;
    }
    case GPSType::Femto:
        baseStationDriver = new GPSDriverFemto(&_callbackEntry, this, &_sensorGps, &_satelliteInfo);
        gpsDriver.reset(baseStationDriver);
        baudrate = 0;
        break;
    case GPSType::NMEA:
        gpsDriver.reset(new GPSDriverNMEA(&_callbackEntry, this, &_sensorGps, &_satelliteInfo, _rtkData.headingOffset));
        baudrate = NMEA_DEFAULT_BAUDRATE;
        break;
    case GPSType::MTK:
        gpsDriver.reset(new GPSDriverMTK(&_callbackEntry, this, &_sensorGps));
        baudrate = 0;
        break;
    case GPSType::EmlidReach:
        gpsDriver.reset(new GPSDriverEmlidReach(&_callbackEntry, this, &_sensorGps, &_satelliteInfo));
        baudrate = 0;
        break;
    default:
        qCWarning(GPSProviderLog) << "Unsupported GPS Type:" << static_cast<int>(_type);
        return nullptr;
    }

    if (baseStationDriver) {
        switch (_rtkData.useFixedBaseLocation) {
        case rtk_data_s::BaseFixed:
            baseStationDriver->setBasePosition(_rtkData.fixedBaseLatitude, _rtkData.fixedBaseLongitude, _rtkData.fixedBaseAltitudeMeters, _rtkData.fixedBaseAccuracyMeters * 1000.0f);
            break;
        case rtk_data_s::BaseSurveyIn:
        default:
            baseStationDriver->setSurveyInSpecs(_rtkData.surveyInAccMeters * 10000.f, _rtkData.surveyInDurationSecs);
            break;
        }
        _gpsConfig.output_mode = static_cast<GPSHelper::OutputMode>(_rtkData.outputMode);
    } else {
        _gpsConfig.output_mode = GPSHelper::OutputMode::GPS;
    }

    _gpsConfig.gnss_systems = static_cast<GPSHelper::GNSSSystemsMask>(_rtkData.gnssSystems);

    if (gpsDriver->configure(baudrate, _gpsConfig) != 0) {
        return nullptr;
    }

    return gpsDriver;
}
