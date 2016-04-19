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

#include "GPSManager.h"
#include <QDebug>

GPSManager::GPSManager(QGCApplication* app)
    : QGCTool(app)
{
    qRegisterMetaType<GPSPositionMessage>();
    qRegisterMetaType<GPSSatelliteMessage>();
}

GPSManager::~GPSManager()
{
    cleanup();
}

void GPSManager::setupGPS(const QString& device)
{
    Q_ASSERT(_toolbox);

    cleanup();
    _requestGpsStop = false;
    _gpsProvider = new GPSProvider(device, true, _requestGpsStop);
    _gpsProvider->start();

    //create RTCM device
    _rtcmMavlink = new RTCMMavlink(*_toolbox);

    connect(_gpsProvider, SIGNAL(RTCMDataUpdate(QByteArray)), _rtcmMavlink,
            SLOT(RTCMDataUpdate(QByteArray)));

    //test: connect to position update
    connect(_gpsProvider, SIGNAL(positionUpdate(GPSPositionMessage)), this,
            SLOT(GPSPositionUpdate(GPSPositionMessage)));
    connect(_gpsProvider, SIGNAL(satelliteInfoUpdate(GPSSatelliteMessage)), this,
            SLOT(GPSSatelliteUpdate(GPSSatelliteMessage)));

}

void GPSManager::GPSPositionUpdate(GPSPositionMessage msg)
{
    qDebug("GPS: got position update: alt=%i, long=%i, lat=%i",
            msg.position_data.alt, msg.position_data.lon,
            msg.position_data.lat);
}
void GPSManager::GPSSatelliteUpdate(GPSSatelliteMessage msg)
{
    qDebug("GPS: got satellite info update, %i satellites", (int)msg.satellite_data.count);
}

void GPSManager::cleanup()
{
    if (_gpsProvider) {
        _requestGpsStop = true;
        //Note that we need a relatively high timeout to be sure the GPS thread finished.
        if (!_gpsProvider->wait(2000)) {
            qWarning() << "Failed to wait for GPS thread exit. Consider increasing the timeout";
        }
        delete(_gpsProvider);
    }
    if (_rtcmMavlink) {
        delete(_rtcmMavlink);
    }
}
