#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include "GPSReceiverSession.h"
#include "GPSSatelliteStore.h"
#include "GPSSourceHealth.h"

class GPSReceiverFactGroup;

class GPSReceiver : public QObject
{
    Q_OBJECT

    friend class GPSReceiverTest;

public:
    /// The receiver session must outlive this presentation object.
    explicit GPSReceiver(GPSReceiverSession& session, QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~GPSReceiver();

    bool connected() const;

    GPSSourceHealth* health() { return &_health; }

    bool hasReceiver() const { return _session.hasReceiver(); }

    bool stopping() const { return _session.stopping(); }

    const GPSReceiverCapabilities& capabilities() const { return _session.capabilities(); }

    QString errorDetail() const { return _session.errorDetail(); }

    GPSReceiverFactGroup* facts() const { return _facts; }

signals:
    void diagnosticsChanged();
    void receiverTypeChanged(GPSType type);
    void satellitesReceived(const GPSSatelliteObservation& observation);
    void connectedChanged();
    void receiverStateChanged();
    void configurationStarted();
    void connectionFailed();

private slots:
    void _attemptChanged(const GPSReceiverAttempt& attempt);
    void _satelliteInfoUpdate(const GPSSatelliteObservation& msg);
    void _sensorGpsUpdate(const GPSObservation& msg);
    void _onGPSConnect();
    void _onGPSDisconnect();
    void _onGPSConnectionError(GPSConnectionError error);

private:
    GPSReceiverSession& _session;
    GPSSourceHealth _health;
    GPSSatelliteStore _satellites;
    GPSReceiverFactGroup* _facts = nullptr;
    quint64 _projectionRevision = 0;
};
