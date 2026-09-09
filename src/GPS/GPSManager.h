#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSCorrectionManager.h"
#include "GPSReceiverAutoConnect.h"
#include "NMEASourceManager.h"

class GPSReceiver;
class GPSRtkState;
class QGCPositionManager;
class QTimer;

class GPSManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(NMEASourceManager* nmeaConnection READ nmeaConnection CONSTANT)
    Q_PROPERTY(GPSReceiverAutoConnect* rtkConnection READ rtkConnection CONSTANT)
    Q_PROPERTY(GPSCorrectionManager* corrections READ corrections CONSTANT)
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

    GPSReceiver* receiver() const { return _receiver; }

    GPSRtkState* rtkState() const { return _rtkState; }

    GPSReceiverSession* receiverSession() { return &_receiverSession; }

    NMEASourceManager* nmeaConnection() const { return _nmeaSources; }

    GPSReceiverAutoConnect* rtkConnection() const { return _receiverAutoConnect; }

    GPSCorrectionManager* corrections() { return &_corrections; }

    Q_INVOKABLE bool connectNmea();
    Q_INVOKABLE void disconnectNmea();
    Q_INVOKABLE bool connectRtk();
    Q_INVOKABLE void disconnectRtk();

    bool networkRtkActive() const;
    bool networkRtkAutoConnectPaused() const;

    Q_INVOKABLE bool connectNetworkRtk();
    Q_INVOKABLE void disconnectNetworkRtk();

signals:
    void networkRtkActiveChanged();
    void networkRtkAutoConnectPausedChanged();

private:
    void _updateConnections();
    void _updateReceiverSettings(bool restart = false);
    void _updatePositionSource();
    bool _positionSourceInstalled = false;
    QPointer<QGCPositionManager> _positionManager;
    GPSCorrectionManager _corrections;
    GPSReceiverSession _receiverSession;
    QTimer* _connectionTimer = nullptr;
    NMEASourceManager* _nmeaSources = nullptr;
    GPSReceiverAutoConnect* _receiverAutoConnect = nullptr;
    GPSReceiver* _receiver = nullptr;
    GPSRtkState* _rtkState = nullptr;
};
