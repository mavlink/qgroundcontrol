#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

#include <functional>
#include <optional>

#include "GPSConnectionState.h"
#include "GPSReceiverProfile.h"
#include "GPSReceiverSession.h"
#include "GPSSourceHealth.h"

class SerialPortManager;

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
                                    QObject* parent = nullptr);
    ~GPSReceiverAutoConnect() override;

    void setProfile(const GPSReceiverProfile& profile, bool restart = false);
    void setAutoConnect(bool enabled);
#ifndef QGC_NO_SERIAL_LINK
    using SerialTransportFactory = std::function<GPSProvider::TransportFactory(const QString&)>;
    void setSerialDiscovery(SerialPortManager* serialPorts);
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

    QString validationError() const { return _profile.validationError(); }

    bool active() const { return _connection.active(); }

    GPSConnectionState::State connectionState() const { return _connection.state(); }

    bool autoConnectPaused() const { return _automatic && _connection.paused(); }

    bool networkActive() const;

    bool networkAutoConnectPaused() const { return _connection.paused(); }

signals:
    void stateChanged();
    void networkActiveChanged();
    void networkAutoConnectPausedChanged();
    void connectRequested(const QString& device, const QString& name, const GPSReceiverConfig& config);
    void disconnectRequested();

private:
    bool _serialSelected() const { return _profile.endpoint.kind == GPSReceiverProfile::Endpoint::Kind::Serial; }

    void _stop(quint64 revision);
    void _stopAttempt(quint64 revision);
    bool _captureConfig();
    bool _retryReady();
    void _startReceiver();
    void _updateReceiverState();

    QPointer<GPSReceiverSession> _receiver;
    QPointer<GPSSourceHealth> _health;
    GPSConnectionState _connection;
    GPSReceiverProfile _profile{
        .endpoint = {.kind = GPSReceiverProfile::Endpoint::Kind::Serial, .discoverSerialDevice = true},
        .configurationPolicy = GPSReceiverProfile::ConfigurationPolicy::Configure,
        .receiver = {.base = {.surveyInAccMeters = 2.0, .surveyInDurationSecs = 180}}};
    quint64 _commandRevision = 0;
    quint64 _handledTerminalAttempt = 0;
    bool _automatic = false;
    GPSProvider::TransportFactory _transportFactory;
    std::optional<GPSReceiverProfile> _sessionConfig;
#ifndef QGC_NO_SERIAL_LINK
    void _updateSerial();
    SerialPortManager* _serialPorts = nullptr;
    SerialTransportFactory _serialFactory;
    QString _autoConnectedPort;
    QMap<QString, QElapsedTimer> _waitingPorts;
#ifdef Q_OS_WIN
    int _connectDelayMs = 6000;
#else
    int _connectDelayMs = 1000;
#endif
#endif
};
