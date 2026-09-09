#pragma once

#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSConnectionState.h"
#include "GPSProvider.h"
#include "GPSSourceHealth.h"
#include "RTKConnectionConfig.h"

#include <optional>

class AutoConnectSettings;
class GPSRtk;
class RTKSettings;
class SerialPortManager;

/// RTK session selection and recovery; GPSRtk owns the receiver worker and transport.
class RTKAutoConnect : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(GPSConnectionState::State connectionState READ connectionState NOTIFY stateChanged)
    Q_PROPERTY(GPSSourceHealth* health READ health CONSTANT)
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(bool autoConnectPaused READ autoConnectPaused NOTIFY stateChanged)
    Q_PROPERTY(QString errorDetail READ errorDetail NOTIFY stateChanged)
    friend class RTKAutoConnectTest;

public:
    RTKAutoConnect(GPSRtk* receiver, AutoConnectSettings* settings, RTKSettings* rtkSettings,
                   QObject* parent = nullptr);
    ~RTKAutoConnect() override;
#ifndef QGC_NO_SERIAL_LINK
    RTKAutoConnect(AutoConnectSettings* settings, GPSRtk* receiver, SerialPortManager* serialPorts,
                   QObject* parent = nullptr);
    void setSerialDiscovery(SerialPortManager* serialPorts);
#endif
    bool connectSelected();
    void disconnectSelected();

    GPSSourceHealth* health() const;
    QString errorDetail() const;

    bool active() const { return _connection.active(); }

    GPSConnectionState::State connectionState() const { return _connection.state(); }

    bool autoConnectPaused() const;
    bool connectNetwork();
    bool connectNetwork(GPSType type, GPSProvider::TransportFactory factory);
    void disconnectNetwork();

    bool networkActive() const { return static_cast<bool>(_networkFactory); }

    bool networkAutoConnectPaused() const { return _connection.paused(); }
    void update();
    void stop();

signals:
    void stateChanged();
    void networkActiveChanged();
    void networkAutoConnectPausedChanged();
    void connectRequested(const QString& device, const QString& name, const GPSReceiverConfig& config);
    void disconnectRequested();

private:
    bool _serialSelected() const;
    bool _captureConfig();
    bool _connectNetwork(const RTKConnectionConfig& config, GPSProvider::TransportFactory factory);
    bool _retryReady();
    void _startNetwork();
    void _updateReceiverState();

    GPSRtk* _receiver;
    AutoConnectSettings* _settings;
    RTKSettings* _rtkSettings;
    GPSConnectionState _connection;
    GPSProvider::TransportFactory _networkFactory;
    std::optional<RTKConnectionConfig> _sessionConfig;
#ifndef QGC_NO_SERIAL_LINK
    void _updateSerial();
    SerialPortManager* _serialPorts = nullptr;
    QString _autoConnectedPort;
    QMap<QString, QElapsedTimer> _waitingPorts;
#ifdef Q_OS_WIN
    int _connectDelayMs = 6000;
#else
    int _connectDelayMs = 1000;
#endif
#endif
};
