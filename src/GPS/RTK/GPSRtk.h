#pragma once

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QString>

#include "GPSProvider.h"
#include "GPSSourceHealth.h"
#include "satellite_info.h"
#include "sensor_gps.h"

class GPSRTKFactGroup;
class RTKPositionSource;
class FactGroup;

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
    void connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory);
    void disconnectGPS();
    bool connected() const;

    GPSSourceHealth* health() { return &_health; }

    RTKPositionSource* positionSource() const { return _positionSource; }

    bool hasReceiver() const { return _gpsProvider != nullptr; }

    bool stopping() const { return !_retiringProviders.isEmpty(); }
    FactGroup* gpsRtkFactGroup();

    struct SatelliteCounts
    {
        uint8_t inView = 0;
        int used = 0;
    };

    /// Clamp count to the array bound and tally used-in-solution satellites.
    static SatelliteCounts countSatellites(const satellite_info_s& msg);

signals:
    void rtcmDataReceived(const QByteArray& data);
    void connectedChanged();
    void receiverStateChanged();
    void configurationStarted();
    void connectionFailed();

private slots:
    void _satelliteInfoUpdate(const satellite_info_s& msg);
    void _sensorGpsUpdate(const sensor_gps_s& msg);
    void _onGPSConnect();
    void _onGPSDisconnect();
    void _onGPSConnectionError(GPSConnectionError error);
    void _onGPSSurveyInStatus(const GPSSurveyInStatus& status);

private:
    GPSSourceHealth _health;
    RTKPositionSource* _positionSource = nullptr;
    GPSProvider* _gpsProvider = nullptr;
    GPSRTKFactGroup* _gpsRtkFactGroup = nullptr;

    // Retired workers delete themselves on finished(); this set only gates reconnection.
    QSet<GPSProvider*> _retiringProviders;
};
