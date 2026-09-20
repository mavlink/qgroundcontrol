#pragma once

#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QPointer>

class AutoConnectSettings;
class GPSRtk;
class SerialPortManager;

/// Serial RTK discovery policy; GPSRtk owns the worker and its serial claim.
class RTKAutoConnect : public QObject
{
    Q_OBJECT
    friend class RTKAutoConnectTest;

public:
    RTKAutoConnect(AutoConnectSettings* settings, GPSRtk* receiver, SerialPortManager* serialPorts,
                   QObject* parent = nullptr);
    void update();
    void stop();

signals:
    void connectRequested(const QString& device, const QString& name);
    void disconnectRequested();

private:
    void _resetDiscovery();

    QPointer<AutoConnectSettings> _settings;
    QPointer<GPSRtk> _receiver;
    QPointer<SerialPortManager> _serialPorts;
    quint64 _revision = 0;
    QString _autoConnectedPort;
    QMap<QString, QElapsedTimer> _waitingPorts;
    QDeadlineTimer _retryDeadline = QDeadlineTimer::Forever;
    int _retryDelayMs = 1000;
    static constexpr int kMaxRetryDelayMs = 30000;
#ifdef Q_OS_WIN
    int _connectDelayMs = 6000;
#else
    int _connectDelayMs = 1000;
#endif
};
