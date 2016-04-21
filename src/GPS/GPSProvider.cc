/*=====================================================================

 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#include "GPSProvider.h"

#define TIMEOUT_5HZ 500

#include <QDebug>

#include "Drivers/src/ubx.h"
#include "Drivers/src/gps_helper.h"
#include "definitions.h"


void GPSProvider::run()
{
    if (_serial) delete _serial;

    _serial = new QSerialPort();
    _serial->setPortName(_device);
    if (!_serial->open(QIODevice::ReadWrite)) {
        qWarning() << "GPS: Failed to open Serial Device" << _device;
        return;
    }
    _serial->setBaudRate(QSerialPort::Baud9600);
    _serial->setDataBits(QSerialPort::Data8);
    _serial->setParity(QSerialPort::NoParity);
    _serial->setStopBits(QSerialPort::OneStop);
    _serial->setFlowControl(QSerialPort::NoFlowControl);

    unsigned int baudrate;
    GPSHelper* gpsHelper = nullptr;

    while (!_requestStop) {

        if (gpsHelper) {
            delete gpsHelper;
            gpsHelper = nullptr;
        }

        gpsHelper = new GPSDriverUBX(&callbackEntry, this, &_reportGpsPos, _pReportSatInfo);

        if (gpsHelper->configure(baudrate, GPSHelper::OutputMode::RTCM) == 0) {

            /* reset report */
            memset(&_reportGpsPos, 0, sizeof(_reportGpsPos));

            //In rare cases it can happen that we get an error from the driver (eg. checksum failure) due to
            //bus errors or buggy firmware. In this case we want to try multiple times before giving up.
            int numTries = 0;

            while (!_requestStop && numTries < 3) {
                int helperRet = gpsHelper->receive(TIMEOUT_5HZ);

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
    qDebug() << "Exiting GPS thread";
}

GPSProvider::GPSProvider(const QString& device, bool enableSatInfo, const std::atomic_bool& requestStop)
    : _device(device), _requestStop(requestStop)
{
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
    QByteArray message((char*)data, len);
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
            int timeout = *((int *) data1);
            if (!_serial->waitForReadyRead(timeout))
                return 0; //timeout
            msleep(10); //give some more time to buffer data
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
            qInfo("Survey-in status: %is cur accuracy: %imm valid: %i active: %i",
                status->duration, status->mean_accuracy, (int)(status->flags & 1), (int)((status->flags>>1) & 1));
        }
            break;

        case GPSCallbackType::setClock:
            /* do nothing */
            break;
    }

    return 0;
}
