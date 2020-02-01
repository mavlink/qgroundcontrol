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
#include "QGCApplication.h"
#include "SettingsManager.h"

#define GPS_RECEIVE_TIMEOUT 1200

#include <QDebug>

#include "Drivers/src/ubx.h"
#include "Drivers/src/sbf.h"
#include "Drivers/src/ashtech.h"
#include "Drivers/src/base_station.h"
#include "definitions.h"

//#define SIMULATE_RTCM_OUTPUT //if defined, generate simulated RTCM messages
                               //additionally make sure to call connectGPS(""), eg. from QGCToolbox.cc


void GPSProvider::run()
{
#ifdef SIMULATE_RTCM_OUTPUT
        const int fakeMsgLengths[3] = { 30, 170, 240 };
        uint8_t* fakeData = new uint8_t[fakeMsgLengths[2]];
        while (!_requestStop) {
            for (int i = 0; i < 3; ++i) {
                gotRTCMData((uint8_t*) fakeData, fakeMsgLengths[i]);
                msleep(4);
            }
            msleep(100);
        }
        delete[] fakeData;
#endif /* SIMULATE_RTCM_OUTPUT */

    if (_serial) delete _serial;

    _serial = new QSerialPort();
    _serial->setPortName(_device);
    if (!_serial->open(QIODevice::ReadWrite)) {
        int retries = 60;
        // Give the device some time to come up. In some cases the device is not
        // immediately accessible right after startup for some reason. This can take 10-20s.
        while (retries-- > 0 && _serial->error() == QSerialPort::PermissionError) {
            qCDebug(RTKGPSLog) << "Cannot open device... retrying";
            msleep(500);
            if (_serial->open(QIODevice::ReadWrite)) {
                _serial->clearError();
                break;
            }
        }
        if (_serial->error() != QSerialPort::NoError) {
            qWarning() << "GPS: Failed to open Serial Device" << _device << _serial->errorString();
            return;
        }
    }
    _serial->setBaudRate(QSerialPort::Baud9600);
    _serial->setDataBits(QSerialPort::Data8);
    _serial->setParity(QSerialPort::NoParity);
    _serial->setStopBits(QSerialPort::OneStop);
    _serial->setFlowControl(QSerialPort::NoFlowControl);

    unsigned int baudrate;
    GPSBaseStationSupport* gpsDriver = nullptr;

    while (!_requestStop) {

        if (gpsDriver) {
            delete gpsDriver;
            gpsDriver = nullptr;
        }

        if (_type == GPSType::trimble) {
            gpsDriver = new GPSDriverAshtech(&callbackEntry, this, &_reportGpsPos, _pReportSatInfo);
            baudrate = 115200;
        } else if (_type == GPSType::septentrio) {
            gpsDriver = new GPSDriverSBF(&callbackEntry, this, &_reportGpsPos, _pReportSatInfo, 5);
            baudrate = 0; // auto-configure
        } else {
            gpsDriver = new GPSDriverUBX(GPSDriverUBX::Interface::UART, &callbackEntry, this, &_reportGpsPos, _pReportSatInfo);
            baudrate = 0; // auto-configure
        }
        gpsDriver->setSurveyInSpecs(_surveyInAccMeters * 10000.0f, _surveryInDurationSecs);

        if (_useFixedBaseLoction) {
            gpsDriver->setBasePosition(_fixedBaseLatitude, _fixedBaseLongitude, _fixedBaseAltitudeMeters, _fixedBaseAccuracyMeters * 1000.0f);
        }

        if (gpsDriver->configure(baudrate, GPSDriverUBX::OutputMode::RTCM) == 0) {

            /* reset report */
            memset(&_reportGpsPos, 0, sizeof(_reportGpsPos));

            //In rare cases it can happen that we get an error from the driver (eg. checksum failure) due to
            //bus errors or buggy firmware. In this case we want to try multiple times before giving up.
            int numTries = 0;

            while (!_requestStop && numTries < 3) {
                int helperRet = gpsDriver->receive(GPS_RECEIVE_TIMEOUT);

                if (helperRet > 0) {
                    numTries = 0;

                    if (helperRet & 1) {
                        publishGPSPosition();
                        numTries = 0;
                    }

                    if (_pReportSatInfo && (helperRet & 2)) {
                        publishGPSSatellite();
                        numTries = 0;
                    }
                } else {
                    ++numTries;
                }
            }
            if (_serial->error() != QSerialPort::NoError && _serial->error() != QSerialPort::TimeoutError) {
                break;
            }
        }
    }
    qCDebug(RTKGPSLog) << "Exiting GPS thread";
}

GPSProvider::GPSProvider(const QString& device,
                         GPSType    type,
                         bool       enableSatInfo,
                         double     surveyInAccMeters,
                         int        surveryInDurationSecs,
                         bool       useFixedBaseLocation,
                         double     fixedBaseLatitude,
                         double     fixedBaseLongitude,
                         float      fixedBaseAltitudeMeters,
                         float      fixedBaseAccuracyMeters,
                         const std::atomic_bool& requestStop)
    : _device                   (device)
    , _type                     (type)
    , _requestStop              (requestStop)
    , _surveyInAccMeters        (surveyInAccMeters)
    , _surveryInDurationSecs    (surveryInDurationSecs)
    , _useFixedBaseLoction      (useFixedBaseLocation)
    , _fixedBaseLatitude        (fixedBaseLatitude)
    , _fixedBaseLongitude       (fixedBaseLongitude)
    , _fixedBaseAltitudeMeters  (fixedBaseAltitudeMeters)
    , _fixedBaseAccuracyMeters  (fixedBaseAccuracyMeters)
{
    qCDebug(RTKGPSLog) << "Survey in accuracy:duration" << surveyInAccMeters << surveryInDurationSecs;
    if (enableSatInfo) _pReportSatInfo = new satellite_info_s();
}

GPSProvider::~GPSProvider()
{
    if (_pReportSatInfo) delete(_pReportSatInfo);
    if (_serial) delete _serial;
}

void GPSProvider::publishGPSPosition()
{
    GPSPositionMessage msg;
    msg.position_data = _reportGpsPos;
    emit positionUpdate(msg);
}

void GPSProvider::publishGPSSatellite()
{
    GPSSatelliteMessage msg;
    msg.satellite_data = *_pReportSatInfo;
    emit satelliteInfoUpdate(msg);
}

void GPSProvider::gotRTCMData(uint8_t* data, size_t len)
{
    QByteArray message((char*)data, static_cast<int>(len));
    emit RTCMDataUpdate(message);
}

int GPSProvider::callbackEntry(GPSCallbackType type, void *data1, int data2, void *user)
{
    GPSProvider *gps = (GPSProvider *)user;
    return gps->callback(type, data1, data2);
}

int GPSProvider::callback(GPSCallbackType type, void *data1, int data2)
{
    switch (type) {
        case GPSCallbackType::readDeviceData: {
            if (_serial->bytesAvailable() == 0) {
                int timeout = *((int *) data1);
                if (!_serial->waitForReadyRead(timeout))
                    return 0; //timeout
            }
            return (int)_serial->read((char*) data1, data2);
        }
        case GPSCallbackType::writeDeviceData:
            if (_serial->write((char*) data1, data2) >= 0) {
                if (_serial->waitForBytesWritten(-1))
                    return data2;
            }
            return -1;

        case GPSCallbackType::setBaudrate:
            return _serial->setBaudRate(data2) ? 0 : -1;

        case GPSCallbackType::gotRTCMMessage:
            gotRTCMData((uint8_t*) data1, data2);
            break;

        case GPSCallbackType::surveyInStatus:
        {
            SurveyInStatus* status = (SurveyInStatus*)data1;
            qCDebug(RTKGPSLog) << "Position: " << status->latitude << status->longitude << status->altitude;

            qCDebug(RTKGPSLog) << QString("Survey-in status: %1s cur accuracy: %2mm valid: %3 active: %4").arg(status->duration).arg(status->mean_accuracy).arg((int)(status->flags & 1)).arg((int)((status->flags>>1) & 1));
            emit surveyInStatus(status->duration, status->mean_accuracy, status->latitude, status->longitude, status->altitude, (int)(status->flags & 1), (int)((status->flags>>1) & 1));
        }
            break;

        case GPSCallbackType::setClock:
            /* do nothing */
            break;
    }

    return 0;
}
