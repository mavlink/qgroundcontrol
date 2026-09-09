#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include "GPSReceiverSession.h"
#include "GPSSourceHealth.h"

class GPSRTKFactGroup;
class GPSReceiverPositionSource;
class FactGroup;

class GPSRtk : public QObject
{
    Q_OBJECT

    friend class GPSRtkTest;

public:
    explicit GPSRtk(QObject* parent = nullptr);
    ~GPSRtk();

#ifndef QGC_NO_SERIAL_LINK
    void connectGPS(const QString& device, QStringView gps_type, GPSReceiverConfig config);
#endif
    void connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory, GPSReceiverConfig config);
    void disconnectGPS();
    /// Final application teardown: join cancelled workers without relying on the event loop.
    void shutdown();
    bool connected() const;

    GPSSourceHealth* health() { return &_health; }

    GPSReceiverPositionSource* positionSource() const { return _positionSource; }

    bool hasReceiver() const { return _session.hasReceiver(); }

    bool stopping() const { return _session.stopping(); }

    const GPSReceiverCapabilities& capabilities() const { return _session.capabilities(); }

    QString errorDetail() const { return _session.errorDetail(); }
    FactGroup* gpsRtkFactGroup();

    struct SatelliteCounts
    {
        uint8_t inView = 0;
        int used = 0;
    };

    /// Clamp count to the array bound and tally used-in-solution satellites.
    static SatelliteCounts countSatellites(const GPSSatelliteObservation& msg);

signals:
    void diagnosticsChanged();
    void receiverTypeChanged(GPSType type);
    void relativePositionReceived(const GPSRelativeObservation& observation);
    void rtcmDataReceived(const QByteArray& data);
    void rtcmFrameReceived(const QByteArray& data, qint64 receivedAtMs);
    void connectedChanged();
    void receiverStateChanged();
    void configurationStarted();
    void connectionFailed();

private slots:
    void _satelliteInfoUpdate(const GPSSatelliteObservation& msg);
    void _sensorGpsUpdate(const GPSObservation& msg);
    void _onGPSConnect();
    void _onGPSDisconnect();
    void _onGPSConnectionError(GPSConnectionError error);
    void _onGPSSurveyInStatus(const GPSSurveyInStatus& status);

private:
    GPSReceiverSession _session;
    GPSSourceHealth _health;
    GPSReceiverPositionSource* _positionSource = nullptr;
    GPSRTKFactGroup* _gpsRtkFactGroup = nullptr;
};
