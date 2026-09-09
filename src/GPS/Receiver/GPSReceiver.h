#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include "GPSReceiverSession.h"
#include "GPSSourceHealth.h"

class GPSReceiverFactGroup;
class GPSReceiverPositionSource;

class GPSReceiver : public QObject
{
    Q_OBJECT

    friend class GPSReceiverTest;

public:
    /// The receiver session must outlive this presentation object.
    explicit GPSReceiver(GPSReceiverSession& session, QObject* parent = nullptr);
    ~GPSReceiver();

    bool connected() const;

    GPSSourceHealth* health() { return &_health; }

    GPSReceiverPositionSource* positionSource() const { return _positionSource; }

    bool hasReceiver() const { return _session.hasReceiver(); }

    bool stopping() const { return _session.stopping(); }

    const GPSReceiverCapabilities& capabilities() const { return _session.capabilities(); }

    QString errorDetail() const { return _session.errorDetail(); }

    GPSReceiverFactGroup* facts() const { return _facts; }

    struct SatelliteCounts
    {
        uint8_t inView = 0;
        int used = 0;
    };

    /// Count the satellites in view and used in the solution.
    static SatelliteCounts countSatellites(const GPSSatelliteObservation& msg);

signals:
    void diagnosticsChanged();
    void receiverTypeChanged(GPSType type);
    void relativePositionReceived(const GPSRelativeObservation& observation);
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

private:
    GPSReceiverSession& _session;
    GPSSourceHealth _health;
    GPSReceiverPositionSource* _positionSource = nullptr;
    GPSReceiverFactGroup* _facts = nullptr;
};
