/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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

void GPSManager::connectGPS(const QString& device)
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
