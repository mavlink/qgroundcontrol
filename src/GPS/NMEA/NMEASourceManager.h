#pragma once

#include <QtCore/QDeadlineTimer>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoSatelliteInfo>
#include <QtQmlIntegration/QtQmlIntegration>

#include <deque>
#include <functional>
#include <memory>

#include "GPSConnectionState.h"
#include "NMEAConnectionAttempt.h"
#include "NMEAConnectionConfig.h"
#include "NMEADecoderSession.h"

#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
class QSerialPort;
#endif

class AutoConnectSettings;
class QGCPositionManager;
class QTcpSocket;
class UdpIODevice;
class NMEAStreamSplitter;
class NMEAPositionSource;
class NMEASatelliteAdapter;
class QGeoPositionInfoSource;
class QIODevice;
class QNmeaSatelliteInfoSource;

/// Owns one NMEA connection and both decoders; PositionManager borrows the position source.
class NMEASourceManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(GPSConnectionState::State connectionState READ connectionState NOTIFY stateChanged)
    Q_PROPERTY(GPSSourceHealth* health READ health CONSTANT)
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(int satellitesInViewCount READ satellitesInViewCount NOTIFY satellitesChanged)
    Q_PROPERTY(int satellitesInUseCount READ satellitesInUseCount NOTIFY satellitesChanged)
    friend class NMEASourceManagerTest;

public:
    NMEASourceManager(AutoConnectSettings* settings, QGCPositionManager* positionManager, QObject* parent = nullptr);
    ~NMEASourceManager() override;
    void update();
    void stop();
    void shutdown();
    bool connectSource();
    void disconnectSource();
#ifndef QGC_NO_SERIAL_LINK
    /// An explicit null inventory disables serial discovery for this source.
    void setSerialDiscovery(SerialPortManager* serialPorts);
#endif

    QGeoPositionInfoSource* positionSource() const;

    GPSSourceHealth* health() { return _decoder.health(); }

    bool active() const { return _connection.active(); }

    GPSConnectionState::State connectionState() const { return _connection.state(); }

    QString status() const { return _status; }

    /// Counts are -1 until fresh satellite information is available.
    int satellitesInViewCount() const { return _decoder.health()->satellitesInViewCount(); }

    int satellitesInUseCount() const { return _decoder.health()->satellitesInUseCount(); }

    QList<QGeoSatelliteInfo> satellitesInView() const { return _decoder.satellitesInView(); }

    QList<QGeoSatelliteInfo> satellitesInUse() const { return _decoder.satellitesInUse(); }

signals:
    void stateChanged();
    void satellitesChanged();

private:
    bool _shouldConnect() const;
    void _dispatch(std::function<void()> command);
    void _update();
    void _stop();
    void _closeDevice();
    void _setStatus(const QString& status);
    void _settingsChanged();
    void _updateSerialRouting();
    bool _installSource(QIODevice* device);
    void _startAttempt();
    void _uninstallSource();
    void _attemptFailed(const QString& detail);
    void _notifyState();

    AutoConnectSettings* _settings;
    NMEAConnectionConfig _config;
    QPointer<QGCPositionManager> _positionManager;
    std::unique_ptr<NMEAConnectionAttempt> _attempt;
    NMEADecoderSession _decoder;
    QTimer _udpActivityTimer;
    GPSConnectionState _connection;
    GPSProvider::TransportFactory _receiverFactory;
    bool _sourceInstalled = false;
    bool _dispatching = false;
    bool _stateNotificationPending = false;
    bool _shutdown = false;
    std::deque<std::function<void()>> _commands;
    QString _status;
#ifndef QGC_NO_SERIAL_LINK
    QPointer<SerialPortManager> _serialPorts;
    SerialPortManager::ReservationPtr _autoConnectExclusion;
#endif
};
