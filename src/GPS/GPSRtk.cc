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
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "RTKSettings.h"
#include "GPSRTKFactGroup.h"
#include "RTCMMavlink.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRtkLog, "qgc.gps.gpsrtk")

GPSRtk::GPSRtk(QObject* parent)
    : QObject(parent)
{
    qCDebug(GPSRtkLog) << Q_FUNC_INFO << this;

    qRegisterMetaType<satellite_info_s>();
    qRegisterMetaType<sensor_gnss_relative_s>();
    qRegisterMetaType<sensor_gps_s>();

    (void) connect(this, &GPSRtk::onConnect,        this, &GPSRtk::_onGPSConnect);
    (void) connect(this, &GPSRtk::satelliteUpdate,  this, &GPSRtk::_onSatelliteUpdate);
}

GPSRtk::~GPSRtk()
{
    disconnectGPS();

    qCDebug(GPSRtkLog) << Q_FUNC_INFO << this;
}

void GPSRtk::_onGPSConnect()
{
    m_gpsRtkFactGroup->connected()->setRawValue(true);
}

void GPSRtk::_onGPSDisconnect()
{
    m_gpsRtkFactGroup->connected()->setRawValue(false);
}

void GPSRtk::_onGPSSurveyInStatus(float duration, float accuracyMM,  double latitude, double longitude, float altitude, bool valid, bool active)
{
    m_gpsRtkFactGroup->currentDuration()->setRawValue(duration);
    m_gpsRtkFactGroup->currentAccuracy()->setRawValue(static_cast<double>(accuracyMM) / 1000.0);
    m_gpsRtkFactGroup->currentLatitude()->setRawValue(latitude);
    m_gpsRtkFactGroup->currentLongitude()->setRawValue(longitude);
    m_gpsRtkFactGroup->currentAltitude()->setRawValue(altitude);
    m_gpsRtkFactGroup->valid()->setRawValue(valid);
    m_gpsRtkFactGroup->active()->setRawValue(active);
}

void GPSRtk::_onSatelliteUpdate(int numSats)
{
    m_gpsRtkFactGroup->numSatellites()->setRawValue(numSats);
}

void GPSRtk::connectGPS(const QString& device, QStringView gps_type)
{
    RTKSettings* const rtkSettings = qgcApp()->toolbox()->settingsManager()->rtkSettings();

    GPSProvider::GPSType type;
    if (gps_type.contains(QStringLiteral("trimble"),  Qt::CaseInsensitive)) {
        type = GPSProvider::GPSType::trimble;
        qCDebug(GPSRtkLog) << "Connecting Trimble device";
    } else if (gps_type.contains(QStringLiteral("septentrio"),  Qt::CaseInsensitive)) {
        type = GPSProvider::GPSType::septentrio;
        qCDebug(GPSRtkLog) << "Connecting Septentrio device";
    } else {
        type = GPSProvider::GPSType::u_blox;
        qCDebug(GPSRtkLog) << "Connecting U-blox device";
    }

    disconnectGPS();
    m_requestGpsStop = false;
    m_gpsProvider = new GPSProvider(
        device,
        type,
        rtkSettings->surveyInAccuracyLimit()->rawValue().toDouble(),
        rtkSettings->surveyInMinObservationDuration()->rawValue().toInt(),
        rtkSettings->useFixedBasePosition()->rawValue().toBool(),
        rtkSettings->fixedBasePositionLatitude()->rawValue().toDouble(),
        rtkSettings->fixedBasePositionLongitude()->rawValue().toDouble(),
        rtkSettings->fixedBasePositionAltitude()->rawValue().toFloat(),
        rtkSettings->fixedBasePositionAccuracy()->rawValue().toFloat(),
        m_requestGpsStop
    );
    m_gpsProvider->start();

    m_rtcmMavlink = new RTCMMavlink(this);

    (void) connect(m_gpsProvider, &GPSProvider::RTCMDataUpdate,        m_rtcmMavlink,  &RTCMMavlink::RTCMDataUpdate);
    (void) connect(m_gpsProvider, &GPSProvider::sensorGpsUpdate,       this,           &GPSRtk::_sensorGpsUpdate);
    (void) connect(m_gpsProvider, &GPSProvider::satelliteInfoUpdate,   this,           &GPSRtk::_satelliteInfoUpdate);
    (void) connect(m_gpsProvider, &GPSProvider::finished,              this,           &GPSRtk::_onGPSDisconnect);
    (void) connect(m_gpsProvider, &GPSProvider::surveyInStatus,        this,           &GPSRtk::_onGPSSurveyInStatus);

    emit onConnect();
}

void GPSRtk::disconnectGPS()
{
    if (m_gpsProvider) {
        m_requestGpsStop = true;
        if (!m_gpsProvider->wait(2000)) {
            qCWarning(GPSRtkLog) << "Failed to wait for GPS thread exit. Consider increasing the timeout";
        }

        delete m_gpsProvider;
        m_gpsProvider = nullptr;
    }

    if (m_rtcmMavlink) {
        delete m_rtcmMavlink;
        m_rtcmMavlink = nullptr;
    }
}

bool GPSRtk::connected() const
{
    return (m_gpsProvider && m_gpsProvider->isRunning());
}

FactGroup* GPSRtk::gpsRtkFactGroup()
{
    return m_gpsRtkFactGroup;
}

void GPSRtk::_satelliteInfoUpdate(const satellite_info_s& msg)
{
    qCDebug(GPSRtkLog) << Q_FUNC_INFO << QString("%1 satellites").arg(msg.count);
    emit satelliteUpdate(msg.count);
}

void GPSRtk::_sensorGnssRelativeUpdate(const sensor_gnss_relative_s& msg)
{
    qCDebug(GPSRtkLog) << Q_FUNC_INFO;
}

void GPSRtk::_sensorGpsUpdate(const sensor_gps_s& msg)
{
    qCDebug(GPSRtkLog) << Q_FUNC_INFO << QString("alt=%1, long=%2, lat=%3").arg(msg.altitude_msl_m).arg(msg.longitude_deg).arg(msg.latitude_deg);
}
