#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include "GPSCorrectionSourceRegistration.h"
#include "GPSProvider.h"

class GPSRTKFactGroup;
class FactGroup;
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
    FactGroup* gpsRtkFactGroup();

    struct SatelliteCounts
    {
        uint16_t inView = 0;
        int used = 0;
    };

    /// Clamp count to the array bound and tally used-in-solution satellites.
    static SatelliteCounts countSatellites(const GPSSatelliteReport& msg);

private slots:
    void _satelliteInfoUpdate(const GPSSatelliteReport& msg);
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

    unsigned long _disconnectTimeoutMs = 2000;
};
