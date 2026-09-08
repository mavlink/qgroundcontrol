#pragma once

#include <QtCore/QDeadlineTimer>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSType.h"

class GPSRtk;
class NmeaSourceManager;
class RTKAutoConnect;
class QTimer;

class GPSManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(bool networkRtkActive READ networkRtkActive NOTIFY networkRtkActiveChanged)

    friend class GPSManagerTest;

public:
    GPSManager(QObject *parent = nullptr);
    ~GPSManager();

    static GPSManager *instance();

    void init();
    void shutdown();

    GPSRtk *gpsRtk() { return _gpsRtk; }

    bool networkRtkActive() const { return _networkRtkActive; }

    Q_INVOKABLE bool connectNetworkRtk();
    Q_INVOKABLE void disconnectNetworkRtk();

signals:
    void networkRtkActiveChanged();

private:
    void _updateConnections();
    void _updateNetworkRtk();
    void _startNetworkRtk();
    QTimer* _connectionTimer = nullptr;
    NmeaSourceManager* _nmeaSources = nullptr;
#ifndef QGC_NO_SERIAL_LINK
    RTKAutoConnect* _rtkAutoConnect = nullptr;
#endif
    GPSRtk *_gpsRtk = nullptr;
    bool _networkRtkActive = false;
    QString _networkHost;
    quint16 _networkPort = 0;
    GPSType _networkType = GPSType::u_blox;
    QDeadlineTimer _networkRetryDeadline = QDeadlineTimer::Forever;
    int _networkRetryDelayMs = 1000;
    static constexpr int kMaxNetworkRetryDelayMs = 30000;
};
