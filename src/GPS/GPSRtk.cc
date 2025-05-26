/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GPSRtk.h"
#include "GPSProvider.h"
#include "GPSRTKFactGroup.h"
#include "QGCLoggingCategory.h"
#include "RTCMMavlink.h"
#include "RTKSettings.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(GPSRtkLog, "qgc.gps.gpsrtk")

GPSRtk::GPSRtk(QObject *parent)
    : QObject(parent)
    , _gpsRtkFactGroup(new GPSRTKFactGroup(this))
{
    // qCDebug(GPSRtkLog) << Q_FUNC_INFO << this;
}

GPSRtk::~GPSRtk()
{
    disconnectGPS();

    // qCDebug(GPSRtkLog) << Q_FUNC_INFO << this;
}

void GPSRtk::registerQmlTypes()
{
    (void) qRegisterMetaType<satellite_info_s>("satellite_info_s");
    (void) qRegisterMetaType<sensor_gnss_relative_s>("sensor_gnss_relative_s");
    (void) qRegisterMetaType<sensor_gps_s>("sensor_gps_s");
}

void GPSRtk::_onGPSConnect()
{
    _gpsRtkFactGroup->connected()->setRawValue(true);
}

void GPSRtk::_onGPSDisconnect()
{
    _gpsRtkFactGroup->connected()->setRawValue(false);
}

void GPSRtk::_onGPSSurveyInStatus(float duration, float accuracyMM,  double latitude, double longitude, float altitude, bool valid, bool active)
{
    _gpsRtkFactGroup->currentDuration()->setRawValue(duration);
    _gpsRtkFactGroup->currentAccuracy()->setRawValue(static_cast<double>(accuracyMM) / 1000.0);
    _gpsRtkFactGroup->currentLatitude()->setRawValue(latitude);
    _gpsRtkFactGroup->currentLongitude()->setRawValue(longitude);
    _gpsRtkFactGroup->currentAltitude()->setRawValue(altitude);
    _gpsRtkFactGroup->valid()->setRawValue(valid);
    _gpsRtkFactGroup->active()->setRawValue(active);
}

void GPSRtk::connectGPS(const QString &device, QStringView gps_type)
{
    GPSProvider::GPSType type;
    if (gps_type.contains(QStringLiteral("trimble"), Qt::CaseInsensitive)) {
        type = GPSProvider::GPSType::trimble;
        qCDebug(GPSRtkLog) << "Connecting Trimble device";
    } else if (gps_type.contains(QStringLiteral("septentrio"), Qt::CaseInsensitive)) {
        type = GPSProvider::GPSType::septentrio;
        qCDebug(GPSRtkLog) << "Connecting Septentrio device";
    } else if (gps_type.contains(QStringLiteral("femtomes"), Qt::CaseInsensitive)) {
        type = GPSProvider::GPSType::femto;
        qCDebug(GPSRtkLog) << "Connecting Femtomes device";
    } else {
        type = GPSProvider::GPSType::u_blox;
        qCDebug(GPSRtkLog) << "Connecting U-blox device";
    }

    disconnectGPS();

    RTKSettings* const rtkSettings = SettingsManager::instance()->rtkSettings();
    _requestGpsStop = false;
    const GPSProvider::rtk_data_s rtkData = {
        rtkSettings->surveyInAccuracyLimit()->rawValue().toDouble(),
        rtkSettings->surveyInMinObservationDuration()->rawValue().toInt(),
        rtkSettings->useFixedBasePosition()->rawValue().toBool(),
        rtkSettings->fixedBasePositionLatitude()->rawValue().toDouble(),
        rtkSettings->fixedBasePositionLongitude()->rawValue().toDouble(),
        rtkSettings->fixedBasePositionAltitude()->rawValue().toFloat(),
        rtkSettings->fixedBasePositionAccuracy()->rawValue().toFloat()
    };
    _gpsProvider = new GPSProvider(
        device,
        type,
        rtkData,
        _requestGpsStop,
        this
    );
    (void) QMetaObject::invokeMethod(_gpsProvider, "start", Qt::AutoConnection);

    _rtcmMavlink = new RTCMMavlink(this);
    (void) connect(_gpsProvider, &GPSProvider::RTCMDataUpdate, _rtcmMavlink, &RTCMMavlink::RTCMDataUpdate);

    (void) connect(_gpsProvider, &GPSProvider::satelliteInfoUpdate, this, &GPSRtk::_satelliteInfoUpdate);
    (void) connect(_gpsProvider, &GPSProvider::sensorGpsUpdate, this, &GPSRtk::_sensorGpsUpdate);
    (void) connect(_gpsProvider, &GPSProvider::surveyInStatus, this, &GPSRtk::_onGPSSurveyInStatus);
    (void) connect(_gpsProvider, &GPSProvider::finished, this, &GPSRtk::_onGPSDisconnect);

    (void) QMetaObject::invokeMethod(this, "_onGPSConnect", Qt::AutoConnection);
}

void GPSRtk::disconnectGPS()
{
    if (_gpsProvider) {
        _requestGpsStop = true;
        if (!_gpsProvider->wait(kGPSThreadDisconnectTimeout)) {
            qCWarning(GPSRtkLog) << "Failed to wait for GPS thread exit. Consider increasing the timeout";
        }

        _gpsProvider->deleteLater();
        _gpsProvider = nullptr;
    }

    if (_rtcmMavlink) {
        _rtcmMavlink->deleteLater();
        _rtcmMavlink = nullptr;
    }
}

bool GPSRtk::connected() const
{
    return (_gpsProvider ? _gpsProvider->isRunning() : false);
}

FactGroup *GPSRtk::gpsRtkFactGroup()
{
    return _gpsRtkFactGroup;
}

void GPSRtk::_satelliteInfoUpdate(const satellite_info_s &msg)
{
    qCDebug(GPSRtkLog) << Q_FUNC_INFO << QStringLiteral("%1 satellites").arg(msg.count);
    _gpsRtkFactGroup->numSatellites()->setRawValue(msg.count);
}

void GPSRtk::_sensorGnssRelativeUpdate(const sensor_gnss_relative_s &msg)
{
    qCDebug(GPSRtkLog) << Q_FUNC_INFO;
}

void GPSRtk::_sensorGpsUpdate(const sensor_gps_s &msg)
{
    qCDebug(GPSRtkLog) << Q_FUNC_INFO << QStringLiteral("alt=%1, long=%2, lat=%3").arg(msg.altitude_msl_m).arg(msg.longitude_deg).arg(msg.latitude_deg);
}
