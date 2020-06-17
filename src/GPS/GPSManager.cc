/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
    disconnectGPS();
}

void GPSManager::connectGPS(const QString& device, const QString& gps_type)
{
    RTKSettings* rtkSettings = qgcApp()->toolbox()->settingsManager()->rtkSettings();

    GPSProvider::GPSType type;
    if (gps_type.contains("trimble",  Qt::CaseInsensitive)) {
        type = GPSProvider::GPSType::trimble;
        qCDebug(RTKGPSLog) << "Connecting Trimble device";
    } else if (gps_type.contains("septentrio",  Qt::CaseInsensitive)) {
        type = GPSProvider::GPSType::septentrio;
        qCDebug(RTKGPSLog) << "Connecting Septentrio device";
    } else {
        type = GPSProvider::GPSType::u_blox;
        qCDebug(RTKGPSLog) << "Connecting U-blox device";
    }

    disconnectGPS();
    _requestGpsStop = false;
    _gpsProvider = new GPSProvider(device,
                                   type,
                                   true,    /* enableSatInfo */
                                   rtkSettings->surveyInAccuracyLimit()->rawValue().toDouble(),
                                   rtkSettings->surveyInMinObservationDuration()->rawValue().toInt(),
                                   rtkSettings->useFixedBasePosition()->rawValue().toBool(),
                                   rtkSettings->fixedBasePositionLatitude()->rawValue().toDouble(),
                                   rtkSettings->fixedBasePositionLongitude()->rawValue().toDouble(),
                                   rtkSettings->fixedBasePositionAltitude()->rawValue().toFloat(),
                                   rtkSettings->fixedBasePositionAccuracy()->rawValue().toFloat(),
                                   _requestGpsStop);
    _gpsProvider->start();

    //create RTCM device
    _rtcmMavlink = new RTCMMavlink(*_toolbox);

    connect(_gpsProvider, &GPSProvider::RTCMDataUpdate, _rtcmMavlink, &RTCMMavlink::RTCMDataUpdate);

    //test: connect to position update
    connect(_gpsProvider, &GPSProvider::positionUpdate,         this, &GPSManager::GPSPositionUpdate);
    connect(_gpsProvider, &GPSProvider::satelliteInfoUpdate,    this, &GPSManager::GPSSatelliteUpdate);
    connect(_gpsProvider, &GPSProvider::finished,               this, &GPSManager::onDisconnect);
    connect(_gpsProvider, &GPSProvider::surveyInStatus,         this, &GPSManager::surveyInStatus);

    emit onConnect();
}

void GPSManager::disconnectGPS(void)
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
    _gpsProvider = nullptr;
    _rtcmMavlink = nullptr;
}


void GPSManager::GPSPositionUpdate(GPSPositionMessage msg)
{
    qCDebug(RTKGPSLog) << QString("GPS: got position update: alt=%1, long=%2, lat=%3").arg(msg.position_data.alt).arg(msg.position_data.lon).arg(msg.position_data.lat);
}
void GPSManager::GPSSatelliteUpdate(GPSSatelliteMessage msg)
{
    qCDebug(RTKGPSLog) << QString("GPS: got satellite info update, %1 satellites").arg((int)msg.satellite_data.count);
    emit satelliteUpdate(msg.satellite_data.count);
}
