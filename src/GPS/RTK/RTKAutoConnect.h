#pragma once

#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMap>
#include <QtCore/QObject>

#include "GPSProvider.h"

class AutoConnectSettings;
class GPSRtk;
class RTKSettings;
class SerialPortManager;

/// RTK session selection and recovery; GPSRtk owns the receiver worker and transport.
class RTKAutoConnect : public QObject
{
    Q_OBJECT
    friend class RTKAutoConnectTest;

public:
    RTKAutoConnect(GPSRtk* receiver, AutoConnectSettings* settings, RTKSettings* rtkSettings,
                   QObject* parent = nullptr);
#ifndef QGC_NO_SERIAL_LINK
    RTKAutoConnect(AutoConnectSettings* settings, GPSRtk* receiver, SerialPortManager* serialPorts,
                   QObject* parent = nullptr);
    void setSerialDiscovery(SerialPortManager* serialPorts);
#endif
    bool connectNetwork();
    bool connectNetwork(GPSType type, GPSProvider::TransportFactory factory);
    void disconnectNetwork();

    bool networkActive() const { return static_cast<bool>(_networkFactory); }

    bool networkAutoConnectPaused() const { return _networkAutoConnectPaused; }
    void update();
    void stop();

signals:
    void networkActiveChanged();
    void networkAutoConnectPausedChanged();
    void connectRequested(const QString& device, const QString& name);
    void disconnectRequested();

private:
    bool _retryReady();
    void _retryStarted();
    void _startNetwork();
    void _setNetworkAutoConnectPaused(bool paused);

    GPSRtk* _receiver;
    AutoConnectSettings* _settings;
    RTKSettings* _rtkSettings;
    bool _networkAutoConnectPaused = false;
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
