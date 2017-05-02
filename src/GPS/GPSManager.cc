/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "GPSManager.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "RTKSettings.h"

GPSManager::GPSManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
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
    RTKSettings* rtkSettings = qgcApp()->toolbox()->settingsManager()->rtkSettings();

    cleanup();
    _requestGpsStop = false;
    _gpsProvider = new GPSProvider(device, true, rtkSettings->surveyInAccuracyLimit()->rawValue().toDouble(), rtkSettings->surveyInMinObservationDuration()->rawValue().toInt(), _requestGpsStop);
    _gpsProvider->start();

    //create RTCM device
    _rtcmMavlink = new RTCMMavlink(*_toolbox);

    connect(_gpsProvider, &GPSProvider::RTCMDataUpdate, _rtcmMavlink, &RTCMMavlink::RTCMDataUpdate);

    //test: connect to position update
    connect(_gpsProvider, &GPSProvider::positionUpdate, this, &GPSManager::GPSPositionUpdate);
    connect(_gpsProvider, &GPSProvider::satelliteInfoUpdate, this, &GPSManager::GPSSatelliteUpdate);

}

void GPSManager::GPSPositionUpdate(GPSPositionMessage msg)
{
    qCDebug(RTKGPSLog) << QString("GPS: got position update: alt=%1, long=%2, lat=%3").arg(msg.position_data.alt).arg(msg.position_data.lon).arg(msg.position_data.lat);
}
void GPSManager::GPSSatelliteUpdate(GPSSatelliteMessage msg)
{
    qCDebug(RTKGPSLog) << QString("GPS: got satellite info update, %1 satellites").arg((int)msg.satellite_data.count);
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
