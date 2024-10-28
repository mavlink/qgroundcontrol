/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GPSProvider.h"
#include "QGCLoggingCategory.h"
#include "RTCMMavlink.h"

#include <ashtech.h>
#include <base_station.h>
#include <definitions.h>
#include <femtomes.h>
#include <sbf.h>
#include <ubx.h>

#ifdef Q_OS_ANDROID
#include "qserialport.h"
#else
#include <QtSerialPort/QSerialPort>
#endif

QGC_LOGGING_CATEGORY(GPSProviderLog, "qgc.gps.gpsprovider")
QGC_LOGGING_CATEGORY(GPSDriversLog, "qgc.gps.drivers")

GPSProvider::GPSProvider(const QString &device, GPSType type, const rtk_data_s &rtkData, const std::atomic_bool &requestStop, QObject *parent)
    : QThread(parent)
    , _device(device)
    , _type(type)
    , _requestStop(requestStop)
    , _rtkData(rtkData)
{
    // qCDebug(GPSProviderLog) << Q_FUNC_INFO << this;

    qCDebug(GPSProviderLog) << QString("Survey in accuracy: %1 | duration: %2").arg(_rtkData.surveyInAccMeters).arg(_rtkData.surveyInDurationSecs);
}

GPSProvider::~GPSProvider()
{
    delete _serial;

    // qCDebug(GPSProviderLog) << Q_FUNC_INFO << this;
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

int GPSProvider::_callbackEntry(GPSCallbackType type, void *data1, int data2, void *user)
{
    GPSProvider* const gps = reinterpret_cast<GPSProvider*>(user);
    return gps->callback(type, data1, data2);
}

int GPSProvider::callback(GPSCallbackType type, void *data1, int data2)
{
    switch (type) {
    case GPSCallbackType::readDeviceData:
        if (_serial->bytesAvailable() == 0) {
            const int timeout = *(reinterpret_cast<int*>(data1));
            if (!_serial->waitForReadyRead(timeout)) {
                return 0;
            }
        }
        return _serial->read(reinterpret_cast<char*>(data1), data2);
    case GPSCallbackType::writeDeviceData:
        if (_serial->write(reinterpret_cast<char*>(data1), data2) >= 0) {
            if (_serial->waitForBytesWritten(-1)) {
                return data2;
            }
        }
        return -1;
    case GPSCallbackType::setBaudrate:
        return (_serial->setBaudRate(data2) ? 0 : -1);
    case GPSCallbackType::gotRTCMMessage:
        _gotRTCMData(reinterpret_cast<uint8_t*>(data1), data2);
        break;
    case GPSCallbackType::surveyInStatus:
    {
        const SurveyInStatus* const status = reinterpret_cast<SurveyInStatus*>(data1);
        qCDebug(GPSProviderLog) << "Position:" << status->latitude << status->longitude << status->altitude;

        const bool valid = status->flags & 1;
        const bool active = (status->flags >> 1) & 1;

        qCDebug(GPSProviderLog) << QString("Survey-in status: %1s cur accuracy: %2mm valid: %3 active: %4").arg(status->duration).arg(status->mean_accuracy).arg(valid).arg(active);
        emit surveyInStatus(status->duration, status->mean_accuracy, status->latitude, status->longitude, status->altitude, valid, active);
        break;
    }
    case GPSCallbackType::setClock:
    case GPSCallbackType::gotRelativePositionMessage:
        // _sensorGnssRelative
    default:
        break;
    }

    return 0;
}

void GPSProvider::run()
{
#ifdef SIMULATE_RTCM_OUTPUT
    _sendRTCMData();
#endif

    _connectSerial();

    GPSBaseStationSupport *gpsDriver = nullptr;

    while (!_requestStop) {
        if (gpsDriver) {
            delete gpsDriver;
            gpsDriver = nullptr;
        }

        gpsDriver = _connectGPS();
        if (gpsDriver) {
            (void) memset(&_sensorGps, 0, sizeof(_sensorGps));

            uint8_t numTries = 0;
            while (!_requestStop && (numTries < 3)) {
                const int helperRet = gpsDriver->receive(kGPSReceiveTimeout);

                if (helperRet > 0) {
                    numTries = 0;

                    if (helperRet & GPSReceiveType::Position) {
                        _publishSensorGPS();
                        numTries = 0;
                    }

                    if (helperRet & GPSReceiveType::Satellite) {
                        _publishSatelliteInfo();
                        numTries = 0;
                    }
                } else {
                    ++numTries;
                }
            }

            if ((_serial->error() != QSerialPort::NoError) && (_serial->error() != QSerialPort::TimeoutError)) {
                break;
            }
        }
    }

    delete gpsDriver;
    gpsDriver = nullptr;

    delete _serial;
    _serial = nullptr;

    qCDebug(GPSProviderLog) << Q_FUNC_INFO << "Exiting GPS thread";
}

bool GPSProvider::_connectSerial()
{
    _serial = new QSerialPort();
    _serial->setPortName(_device);
    if (!_serial->open(QIODevice::ReadWrite)) {
        // Give the device some time to come up. In some cases the device is not
        // immediately accessible right after startup for some reason. This can take 10-20s.
        uint32_t retries = 60;
        while ((retries-- > 0) && (_serial->error() == QSerialPort::PermissionError)) {
            qCDebug(GPSProviderLog) << "Cannot open device... retrying";
            msleep(500);
            if (_serial->open(QIODevice::ReadWrite)) {
                _serial->clearError();
                break;
            }
        }

        if (_serial->error() != QSerialPort::NoError) {
            qCWarning(GPSProviderLog) << "GPS: Failed to open Serial Device" << _device << _serial->errorString();
            return false;
        }
    }

    (void) _serial->setBaudRate(QSerialPort::Baud9600);
    (void) _serial->setDataBits(QSerialPort::Data8);
    (void) _serial->setParity(QSerialPort::NoParity);
    (void) _serial->setStopBits(QSerialPort::OneStop);
    (void) _serial->setFlowControl(QSerialPort::NoFlowControl);

    return true;
}

GPSBaseStationSupport *GPSProvider::_connectGPS()
{
    GPSBaseStationSupport *gpsDriver = nullptr;
    uint32_t baudrate = 0;
    switch(_type) {
    case GPSType::trimble:
        gpsDriver = new GPSDriverAshtech(&_callbackEntry, this, &_sensorGps, &_satelliteInfo);
        baudrate = 115200;
        break;
    case GPSType::septentrio:
        gpsDriver = new GPSDriverSBF(&_callbackEntry, this, &_sensorGps, &_satelliteInfo, kGPSHeadingOffset);
        baudrate = 0;
        break;
    case GPSType::u_blox:
        gpsDriver = new GPSDriverUBX(GPSDriverUBX::Interface::UART, &_callbackEntry, this, &_sensorGps, &_satelliteInfo);
        baudrate = 0;
        break;
    case GPSType::femto:
        gpsDriver = new GPSDriverFemto(&_callbackEntry, this, &_sensorGps, &_satelliteInfo);
        baudrate = 0;
        break;
    default:
        // GPSDriverEmlidReach, GPSDriverMTK, GPSDriverNMEA
        qCWarning(GPSProviderLog) << "Unsupported GPS Type:" << static_cast<int>(_type);
        return nullptr;
    }

    gpsDriver->setSurveyInSpecs(_rtkData.surveyInAccMeters * 10000.f, _rtkData.surveyInDurationSecs);

    if (_rtkData.useFixedBaseLoction) {
        gpsDriver->setBasePosition(_rtkData.fixedBaseLatitude, _rtkData.fixedBaseLongitude, _rtkData.fixedBaseAltitudeMeters, _rtkData.fixedBaseAccuracyMeters * 1000.0f);
    }

    _gpsConfig.output_mode = GPSHelper::OutputMode::RTCM;

    if (gpsDriver->configure(baudrate, _gpsConfig) != 0) {
        return nullptr;
    }

    return gpsDriver;
}

void GPSProvider::_sendRTCMData()
{
    RTCMMavlink *const rtcm = new RTCMMavlink(this);

    const int fakeMsgLengths[3] = { 30, 170, 240 };
    const uint8_t* const fakeData = new uint8_t[fakeMsgLengths[2]];
    while (!_requestStop) {
        for (int i = 0; i < 3; ++i) {
            const QByteArray message(reinterpret_cast<const char*>(fakeData), fakeMsgLengths[i]);
            rtcm->RTCMDataUpdate(message);
            msleep(4);
        }
        msleep(100);
    }
    delete[] fakeData;
}
