#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include "GPSProvider.h"
#include "satellite_info.h"
#include "sensor_gps.h"

class GPSRTKFactGroup;
class FactGroup;

class GPSRtk : public QObject
{
    Q_OBJECT

public:
    explicit GPSRtk(QObject* parent = nullptr);
    ~GPSRtk();

    void connectGPS(const QString& device, QStringView gps_type);
    void disconnectGPS();
    bool connected() const;
    FactGroup* gpsRtkFactGroup();

    struct SatelliteCounts
    {
        uint8_t inView = 0;
        int used = 0;
    };

    /// Clamp count to the array bound and tally used-in-solution satellites.
    static SatelliteCounts countSatellites(const satellite_info_s& msg);

private slots:
    void _satelliteInfoUpdate(const satellite_info_s& msg);
    void _sensorGpsUpdate(const sensor_gps_s& msg);
    void _onGPSConnect();
    void _onGPSDisconnect();
    void _onGPSConnectionError(GPSConnectionError error);
    void _onGPSSurveyInStatus(const GPSSurveyInStatus& status);

private:
    GPSProvider* _gpsProvider = nullptr;
    GPSRTKFactGroup* _gpsRtkFactGroup = nullptr;

    std::atomic_bool _requestGpsStop = false;

    static constexpr uint32_t kGPSThreadDisconnectTimeout = 2000;
};
