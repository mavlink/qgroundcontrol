#pragma once

#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSProvider.h"

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
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(bool autoConnectPaused READ autoConnectPaused NOTIFY stateChanged)
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

    bool active() const { return networkActive() || _serialRequested; }

    bool autoConnectPaused() const;
    bool connectNetwork();
    bool connectNetwork(GPSType type, GPSProvider::TransportFactory factory);
    void disconnectNetwork();

    bool networkActive() const { return static_cast<bool>(_networkFactory); }

    bool networkAutoConnectPaused() const { return _networkAutoConnectPaused; }
    void update();
    void stop();

signals:
    void stateChanged();
    void networkActiveChanged();
    void networkAutoConnectPausedChanged();
    void connectRequested(const QString& device, const QString& name);
    void disconnectRequested();

private:
    bool _serialSelected() const;
    bool _retryReady();
    void _retryStarted();
    void _startNetwork();
    void _setNetworkAutoConnectPaused(bool paused);

    GPSRtk* _receiver;
    AutoConnectSettings* _settings;
    RTKSettings* _rtkSettings;
    bool _networkAutoConnectPaused = false;
    bool _serialRequested = false;
    bool _serialPaused = false;
    GPSProvider::TransportFactory _networkFactory;
    GPSType _networkType = GPSType::u_blox;
    QDeadlineTimer _retryDeadline = QDeadlineTimer::Forever;
    int _retryDelayMs = 1000;
    static constexpr int kMaxRetryDelayMs = 30000;
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
