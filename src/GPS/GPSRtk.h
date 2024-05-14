/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QString>
#include <QtCore/QObject>
#include <QtCore/QLoggingCategory>

#include "sensor_gps.h"
#include "sensor_gnss_relative.h"
#include "satellite_info.h"

Q_DECLARE_LOGGING_CATEGORY(GPSRtkLog)

class GPSRTKFactGroup;
class FactGroup;
class RTCMMavlink;
class GPSProvider;

class GPSRtk : public QObject
{
    Q_OBJECT

public:
    GPSRtk(QObject* parent = nullptr);
    ~GPSRtk();

    void connectGPS(const QString& device, QStringView gps_type);
    void disconnectGPS();
    bool connected() const;
    FactGroup* gpsRtkFactGroup();

signals:
    void onConnect();
    void satelliteUpdate(int numSats);

private slots:
    void _satelliteInfoUpdate(const satellite_info_s &msg);
    void _sensorGnssRelativeUpdate(const sensor_gnss_relative_s &msg);
    void _sensorGpsUpdate(const sensor_gps_s &msg);
    void _onGPSConnect();
    void _onGPSDisconnect();
    void _onGPSSurveyInStatus(float duration, float accuracyMM, double latitude, double longitude, float altitude, bool valid, bool active);
    void _onSatelliteUpdate(int numSats);

private:
    GPSProvider* m_gpsProvider = nullptr;
    RTCMMavlink* m_rtcmMavlink = nullptr;
    GPSRTKFactGroup* m_gpsRtkFactGroup = nullptr;

    std::atomic_bool m_requestGpsStop = false;
};
