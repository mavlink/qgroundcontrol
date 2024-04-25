/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "GPSManager.h"
#include "GPSProvider.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "RTKSettings.h"
#include "GPSRTKFactGroup.h"
#include "RTCMMavlink.h"
#include "QGCLoggingCategory.h"


GPSManager::GPSManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
    qRegisterMetaType<GPSPositionMessage>();
    qRegisterMetaType<GPSSatelliteMessage>();
}

GPSManager::~GPSManager()
{
    disconnectGPS();

    delete _gpsRtkFactGroup;
    _gpsRtkFactGroup = nullptr;
}

void GPSManager::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);

    _gpsRtkFactGroup = new GPSRTKFactGroup(this);

    connect(this, &GPSManager::onConnect,          this, &GPSManager::_onGPSConnect);
    connect(this, &GPSManager::onDisconnect,       this, &GPSManager::_onGPSDisconnect);
    connect(this, &GPSManager::surveyInStatus,     this, &GPSManager::_gpsSurveyInStatus);
    connect(this, &GPSManager::satelliteUpdate,    this, &GPSManager::_gpsNumSatellites);
}

void GPSManager::_onGPSConnect()
{
    _gpsRtkFactGroup->connected()->setRawValue(true);
}

void GPSManager::_onGPSDisconnect()
{
    _gpsRtkFactGroup->connected()->setRawValue(false);
}

void GPSManager::_gpsSurveyInStatus(float duration, float accuracyMM,  double latitude, double longitude, float altitude, bool valid, bool active)
{
    _gpsRtkFactGroup->currentDuration()->setRawValue(duration);
    _gpsRtkFactGroup->currentAccuracy()->setRawValue(static_cast<double>(accuracyMM) / 1000.0);
    _gpsRtkFactGroup->currentLatitude()->setRawValue(latitude);
    _gpsRtkFactGroup->currentLongitude()->setRawValue(longitude);
    _gpsRtkFactGroup->currentAltitude()->setRawValue(altitude);
    _gpsRtkFactGroup->valid()->setRawValue(valid);
    _gpsRtkFactGroup->active()->setRawValue(active);
}

void GPSManager::_gpsNumSatellites(int numSatellites)
{
    _gpsRtkFactGroup->numSatellites()->setRawValue(numSatellites);
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

bool GPSManager::connected() const { return _gpsProvider && _gpsProvider->isRunning(); }

FactGroup* GPSManager::gpsRtkFactGroup(void) { return _gpsRtkFactGroup; }

void GPSManager::GPSPositionUpdate(GPSPositionMessage msg)
{
    qCDebug(RTKGPSLog) << QString("GPS: got position update: alt=%1, long=%2, lat=%3").arg(msg.position_data.altitude_msl_m).arg(msg.position_data.longitude_deg).arg(msg.position_data.latitude_deg);
}
void GPSManager::GPSSatelliteUpdate(GPSSatelliteMessage msg)
{
    qCDebug(RTKGPSLog) << QString("GPS: got satellite info update, %1 satellites").arg((int)msg.satellite_data.count);
    emit satelliteUpdate(msg.satellite_data.count);
}
