#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

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
    Q_PROPERTY(
        bool networkRtkAutoConnectPaused READ networkRtkAutoConnectPaused NOTIFY networkRtkAutoConnectPausedChanged)

    friend class GPSManagerTest;

public:
    GPSManager(QObject *parent = nullptr);
    ~GPSManager();

    static GPSManager *instance();

    void init();
    void shutdown();

    GPSRtk *gpsRtk() { return _gpsRtk; }

    bool networkRtkActive() const;
    bool networkRtkAutoConnectPaused() const;

    Q_INVOKABLE bool connectNetworkRtk();
    Q_INVOKABLE void disconnectNetworkRtk();

signals:
    void networkRtkActiveChanged();
    void networkRtkAutoConnectPausedChanged();

private:
    void _updateConnections();
    QTimer* _connectionTimer = nullptr;
    NmeaSourceManager* _nmeaSources = nullptr;
    RTKAutoConnect* _rtkAutoConnect = nullptr;
    GPSRtk *_gpsRtk = nullptr;
};
