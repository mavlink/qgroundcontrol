/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GPSProvider.h"
#include "QGCLoggingCategory.h"
#include "Drivers/src/ubx.h"
#include "Drivers/src/sbf.h"
#include "Drivers/src/ashtech.h"
#include "Drivers/src/base_station.h"
#include "definitions.h"

#ifdef Q_OS_ANDROID
#include "qserialport.h"
#else
#include <QtSerialPort/QSerialPort>
#endif

#define GPS_RECEIVE_TIMEOUT 1200

QGC_LOGGING_CATEGORY(GPSProviderLog, "qgc.gps.gpsprovider")

GPSProvider::GPSProvider(
    const QString& device,
    GPSType type,
    double surveyInAccMeters,
    int surveryInDurationSecs,
    bool useFixedBaseLocation,
    double fixedBaseLatitude,
    double fixedBaseLongitude,
    float fixedBaseAltitudeMeters,
    float fixedBaseAccuracyMeters,
    const std::atomic_bool& requestStop,
    QObject* parent)
    : QThread(parent)
    , m_device(device)
    , m_type(type)
    , m_requestStop(requestStop)
    , m_surveyInAccMeters(surveyInAccMeters)
    , m_surveryInDurationSecs(surveryInDurationSecs)
    , m_useFixedBaseLoction(useFixedBaseLocation)
    , m_fixedBaseLatitude(fixedBaseLatitude)
    , m_fixedBaseLongitude(fixedBaseLongitude)
    , m_fixedBaseAltitudeMeters(fixedBaseAltitudeMeters)
    , m_fixedBaseAccuracyMeters(fixedBaseAccuracyMeters)
{
    qCDebug(GPSProviderLog) << Q_FUNC_INFO << this;
    qCDebug(GPSProviderLog) << QString("Survey in accuracy: %1 | duration: %2").arg(surveyInAccMeters).arg(surveryInDurationSecs);
}

GPSProvider::~GPSProvider()
{
    qCDebug(GPSProviderLog) << Q_FUNC_INFO << this;
}

void GPSProvider::_publishSatelliteInfo()
{
    emit satelliteInfoUpdate(m_satelliteInfo);
}

void GPSProvider::_publishSensorGNSSRelative()
{
    emit sensorGnssRelativeUpdate(m_sensorGnssRelative);
}

void GPSProvider::_publishSensorGPS()
{
    emit sensorGpsUpdate(m_sensorGps);
}

void GPSProvider::_gotRTCMData(const uint8_t* data, size_t len)
{
    const QByteArray message(reinterpret_cast<const char*>(data), len);
    emit RTCMDataUpdate(message);
}

int GPSProvider::_callbackEntry(GPSCallbackType type, void *data1, int data2, void *user)
{
    GPSProvider *gps = reinterpret_cast<GPSProvider*>(user);
    return gps->callback(type, data1, data2);
}

int GPSProvider::callback(GPSCallbackType type, void *data1, int data2)
{
    switch (type) {
        case GPSCallbackType::readDeviceData:
            if (m_serial->bytesAvailable() == 0) {
                const int timeout = *(reinterpret_cast<int*>(data1));
                if (!m_serial->waitForReadyRead(timeout)) {
                    return 0;
                }
            }
            return m_serial->read(reinterpret_cast<char*>(data1), data2);

        case GPSCallbackType::writeDeviceData:
            if (m_serial->write(reinterpret_cast<char*>(data1), data2) >= 0) {
                if (m_serial->waitForBytesWritten(-1)) {
                    return data2;
                }
            }
            return -1;

        case GPSCallbackType::setBaudrate:
            return m_serial->setBaudRate(data2) ? 0 : -1;

        case GPSCallbackType::gotRTCMMessage:
            _gotRTCMData(reinterpret_cast<uint8_t*>(data1), data2);
            break;

        case GPSCallbackType::surveyInStatus:
        {
            const SurveyInStatus* status = reinterpret_cast<SurveyInStatus*>(data1);
            qCDebug(GPSProviderLog) << "Position:" << status->latitude << status->longitude << status->altitude;

            const bool valid = status->flags & 1;
            const bool active = (status->flags >> 1) & 1;

            qCDebug(GPSProviderLog) << QString("Survey-in status: %1s cur accuracy: %2mm valid: %3 active: %4").arg(status->duration).arg(status->mean_accuracy).arg(valid).arg(active);
            emit surveyInStatus(status->duration, status->mean_accuracy, status->latitude, status->longitude, status->altitude, valid, active);
            break;
        }

        case GPSCallbackType::setClock:
        case GPSCallbackType::gotRelativePositionMessage:
            // m_sensorGnssRelative
        default:
            break;
    }

    return 0;
}

void GPSProvider::_simulateRTCMOutput()
{
    const int fakeMsgLengths[3] = { 30, 170, 240 };
    uint8_t* fakeData = new uint8_t[fakeMsgLengths[2]];
    while (!m_requestStop) {
        for (int i = 0; i < 3; ++i) {
            _gotRTCMData(fakeData, fakeMsgLengths[i]);
            msleep(4);
        }
        msleep(100);
    }
    delete[] fakeData;
}

void GPSProvider::run()
{
#ifdef SIMULATE_RTCM_OUTPUT
    _simulateRTCMOutput();
#endif
    if (m_serial) {
        delete m_serial;
        m_serial = nullptr;
    }

    m_serial = new QSerialPort(this);
    m_serial->setPortName(m_device);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        uint32_t retries = 60;
        while ((retries-- > 0) && (m_serial->error() == QSerialPort::PermissionError)) {
            qCDebug(GPSProviderLog) << "Cannot open device... retrying";
            msleep(500);
            if (m_serial->open(QIODevice::ReadWrite)) {
                m_serial->clearError();
                break;
            }
        }

        if (m_serial->error() != QSerialPort::NoError) {
            qCWarning(GPSProviderLog) << "GPS: Failed to open Serial Device" << m_device << m_serial->errorString();
            return;
        }
    }

    (void) m_serial->setBaudRate(QSerialPort::Baud9600);
    (void) m_serial->setDataBits(QSerialPort::Data8);
    (void) m_serial->setParity(QSerialPort::NoParity);
    (void) m_serial->setStopBits(QSerialPort::OneStop);
    (void) m_serial->setFlowControl(QSerialPort::NoFlowControl);

    GPSBaseStationSupport* gpsDriver = nullptr;

    while (!m_requestStop) {

        if (gpsDriver) {
            delete gpsDriver;
            gpsDriver = nullptr;
        }

        uint32_t baudrate = 0;

        switch(m_type) {
            case GPSType::trimble:
                gpsDriver = new GPSDriverAshtech(&_callbackEntry, this, &m_sensorGps, &m_satelliteInfo);
                baudrate = 115200;
                break;

            case GPSType::septentrio:
                gpsDriver = new GPSDriverSBF(&_callbackEntry, this, &m_sensorGps, &m_satelliteInfo, 5);
                baudrate = 0;
                break;

            case GPSType::u_blox:
                gpsDriver = new GPSDriverUBX(GPSDriverUBX::Interface::UART, &_callbackEntry, this, &m_sensorGps, &m_satelliteInfo);
                baudrate = 0;
                break;

            default:
                qCWarning(GPSProviderLog) << "Unsupported GPS Type:" << static_cast<int>(m_type);
                break;
        }

        gpsDriver->setSurveyInSpecs(m_surveyInAccMeters * 10000.0f, m_surveryInDurationSecs);

        if (m_useFixedBaseLoction) {
            gpsDriver->setBasePosition(m_fixedBaseLatitude, m_fixedBaseLongitude, m_fixedBaseAltitudeMeters, m_fixedBaseAccuracyMeters * 1000.0f);
        }

        m_gpsConfig.output_mode = GPSHelper::OutputMode::RTCM;

        if (gpsDriver->configure(baudrate, m_gpsConfig) == 0) {
            (void) memset(&m_sensorGps, 0, sizeof(m_sensorGps));

            uint32_t numTries = 0;
            while (!m_requestStop && (numTries < 3)) {
                const int helperRet = gpsDriver->receive(GPS_RECEIVE_TIMEOUT);

                if (helperRet > 0) {
                    numTries = 0;

                    if (helperRet & 1) {
                        _publishSensorGPS();
                        numTries = 0;
                    }

                    if (helperRet & 2) {
                        _publishSatelliteInfo();
                        numTries = 0;
                    }
                } else {
                    ++numTries;
                }
            }

            if ((m_serial->error() != QSerialPort::NoError) && (m_serial->error() != QSerialPort::TimeoutError)) {
                break;
            }
        }
    }

    qCDebug(GPSProviderLog) << Q_FUNC_INFO << "Exiting GPS thread";
}
