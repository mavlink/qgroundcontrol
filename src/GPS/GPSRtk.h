/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>

#include "satellite_info.h"
#include "sensor_gnss_relative.h"
#include "sensor_gps.h"

Q_DECLARE_LOGGING_CATEGORY(GPSRtkLog)

class GPSRTKFactGroup;
class FactGroup;
class RTCMMavlink;
class GPSProvider;

class GPSRtk : public QObject
{
    Q_OBJECT

public:
    GPSRtk(QObject *parent = nullptr);
    ~GPSRtk();

    static void registerQmlTypes();

    void connectGPS(const QString &device, QStringView gps_type);
    void disconnectGPS();
    bool connected() const;
    FactGroup *gpsRtkFactGroup();

private slots:
    void _satelliteInfoUpdate(const satellite_info_s &msg);
    void _sensorGnssRelativeUpdate(const sensor_gnss_relative_s &msg);
    void _sensorGpsUpdate(const sensor_gps_s &msg);
    void _onGPSConnect();
    void _onGPSDisconnect();
    void _onGPSSurveyInStatus(float duration, float accuracyMM, double latitude, double longitude, float altitude, bool valid, bool active);

private:
    GPSProvider *_gpsProvider = nullptr;
    RTCMMavlink *_rtcmMavlink = nullptr;
    GPSRTKFactGroup *_gpsRtkFactGroup = nullptr;

    std::atomic_bool _requestGpsStop = false;

    static constexpr uint32_t kGPSThreadDisconnectTimeout = 2000;
};
