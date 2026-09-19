#pragma once

#include <optional>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include "GPSCorrectionSourceRegistration.h"
#include "GPSProvider.h"

class GPSRTKFactGroup;
class GPSCorrectionManager;

class GPSRtk : public QObject
{
    Q_OBJECT

    friend class GPSRtkTest;

public:
    explicit GPSRtk(QObject* parent = nullptr);
    ~GPSRtk();

#ifndef QGC_NO_SERIAL_LINK
    void connectGPS(const QString& device, QStringView gps_type);
#endif
    /// Inject before connecting; the caller retains ownership.
    void setCorrectionManager(GPSCorrectionManager* manager);
    void connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory,
                         const QString& sourceInstance = {});
    void disconnectGPS();
    bool connected() const;

    bool hasReceiver() const { return _gpsProvider != nullptr; }

    GPSRTKFactGroup* gpsRtkFactGroup();

    struct SatelliteCounts
    {
        uint16_t inView = 0;
        std::optional<int> used;
    };

    /// Usage is exact only for a complete snapshot with every used flag known (including an empty snapshot).
    static SatelliteCounts countSatellites(const GPSSatelliteReport& msg);

private slots:
    void _satelliteInfoUpdate(const GPSSatelliteReport& msg);
    void _satelliteUsageUpdate(const GPSSatelliteUsageReport& msg);
    void _sensorGpsUpdate(const GPSPositionReport& msg);
    void _onGPSConnect();
    void _onGPSDisconnect();
    void _onGPSConnectionError(GPSConnectionError error);
    void _onGPSSurveyInStatus(const GPSSurveyInStatus& status);

private:
    GPSProvider* _gpsProvider = nullptr;
    GPSRTKFactGroup* _gpsRtkFactGroup = nullptr;
    QPointer<GPSCorrectionManager> _correctionManager;
    GPSCorrectionSourceRegistration _correctionRegistration;
    std::optional<GPSPositionReport::FixType> _lastLoggedFixType;

    unsigned long _disconnectTimeoutMs = 2000;
};
