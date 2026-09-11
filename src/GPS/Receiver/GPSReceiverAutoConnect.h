#pragma once

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

#include <functional>
#include <optional>

#include "GPSConnectionControl.h"
#include "GPSReceiverProfile.h"
#include "GPSReceiverSession.h"
#include "GPSSerialDiscovery.h"
#include "GPSSourceHealth.h"

/// Connection intent, discovery and retry policy for one independently owned receiver session.
class GPSReceiverAutoConnect : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(GPSConnectionState::State connectionState READ connectionState NOTIFY stateChanged)
    Q_PROPERTY(GPSSourceHealth* health READ health CONSTANT)
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(bool autoConnectPaused READ autoConnectPaused NOTIFY stateChanged)
    Q_PROPERTY(QString errorDetail READ errorDetail NOTIFY stateChanged)
    Q_PROPERTY(QString validationError READ validationError NOTIFY stateChanged)

    friend class GPSReceiverAutoConnectTest;

public:
    explicit GPSReceiverAutoConnect(GPSReceiverSession* receiver, GPSSourceHealth* health = nullptr,
                                    QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~GPSReceiverAutoConnect() override;

    void setProfile(const GPSReceiverProfile& profile, bool restart = false);
    void setAutoConnect(bool enabled);
    void setSuspended(bool suspended);
#ifndef QGC_NO_SERIAL_LINK
    using SerialTransportFactory = std::function<GPSProvider::TransportFactory(const QString&)>;
    void setSerialDiscovery(GPSSerialDiscovery* serialPorts);
    void setSerialTransportFactory(SerialTransportFactory factory);
#endif
    bool connectSelected();
    void disconnectSelected();
    bool connectNetwork();
    bool connectNetwork(GPSType type, GPSProvider::TransportFactory factory);
    void disconnectNetwork();
    bool connectReceiver(const GPSReceiverProfile& profile, GPSProvider::TransportFactory factory);
    void update();
    void stop();
    /// Cancel the current attempt while preserving the caller's connection intent.
    void stopAttempt();

    GPSSourceHealth* health() const { return _health; }

    QString errorDetail() const;

    QString validationError() const { return _control.profile().validationError(); }

    bool active() const { return _control.connection().active(); }

    GPSConnectionState::State connectionState() const { return _control.connection().state(); }

    bool autoConnectPaused() const { return _control.automatic() && _control.connection().paused(); }

    bool networkActive() const;

    bool networkAutoConnectPaused() const { return _control.connection().paused(); }

signals:
    void stateChanged();
    void networkActiveChanged();
    void networkAutoConnectPausedChanged();
    void connectRequested(const QString& device, const QString& name, const GPSReceiverConfig& config);
    void disconnectRequested();

private:
    bool _serialSelected() const
    {
        return _control.profile().endpoint.kind == GPSReceiverProfile::Endpoint::Kind::Serial;
    }

    void _stop(quint64 revision);
    void _stopAttempt(quint64 revision);
    bool _captureConfig();
    bool _retryReady();
    void _startReceiver();
    bool _startReceiver(const GPSReceiverProfile& profile, GPSProvider::TransportFactory factory,
                        std::function<void()> admitted = {});
    void _updateReceiverState();
    void _scheduleUpdate();

    QPointer<GPSReceiverSession> _receiver;
    QPointer<GPSSourceHealth> _health;
    GPSConnectionControl _control;
    quint64 _handledTerminalAttempt = 0;
    GPSProvider::TransportFactory _transportFactory;
    std::optional<GPSReceiverProfile> _sessionConfig;
#ifndef QGC_NO_SERIAL_LINK
    void _updateSerial();
    QPointer<GPSSerialDiscovery> _serialPorts;
    SerialTransportFactory _serialFactory;
    QString _autoConnectedPort;
    QMap<QString, qint64> _waitingPorts;
#ifdef Q_OS_WIN
    int _connectDelayMs = 6000;
#else
    int _connectDelayMs = 1000;
#endif
#endif
};
