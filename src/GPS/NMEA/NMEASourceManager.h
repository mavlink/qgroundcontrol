#pragma once

#include <QtCore/QDeadlineTimer>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoSatelliteInfo>
#include <QtQmlIntegration/QtQmlIntegration>

#include <memory>

#include "GPSConnectionState.h"
#include "GPSSourceHealth.h"

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
    bool connectSource();
    void disconnectSource();

    QGeoPositionInfoSource* positionSource() const;

    GPSSourceHealth* health() { return &_health; }

    bool active() const { return _connection.active(); }

    GPSConnectionState::State connectionState() const { return _connection.state(); }

    QString status() const { return _status; }

    /// Counts are -1 until fresh satellite information is available.
    int satellitesInViewCount() const { return _health.satellitesInViewCount(); }

    int satellitesInUseCount() const { return _health.satellitesInUseCount(); }

    QList<QGeoSatelliteInfo> satellitesInView() const { return _satellitesInView; }

    QList<QGeoSatelliteInfo> satellitesInUse() const { return _satellitesInUse; }

signals:
    void stateChanged();
    void satellitesChanged();

private:
    bool _shouldConnect() const;
    void _closeDevice();
    void _setStatus(const QString& status);
    void _updateTcp();
    void _tcpFailed(const QString& error);
    void _settingsChanged();
    void _updateSerialRouting();
    bool _installSource(QIODevice* device);
    void _clearSatelliteInfo();

    AutoConnectSettings* _settings;
    QPointer<QGCPositionManager> _positionManager;
    std::unique_ptr<UdpIODevice> _udp;
    std::unique_ptr<QTcpSocket> _tcp;
    std::unique_ptr<NMEAStreamSplitter> _stream;
    std::unique_ptr<NMEAPositionSource> _positionSource;
    std::unique_ptr<QNmeaSatelliteInfoSource> _satelliteSource;
    QTimer _satellitePollTimer;
    GPSSourceHealth _health;
    QList<QGeoSatelliteInfo> _satellitesInView;
    QList<QGeoSatelliteInfo> _satellitesInUse;
    QTimer _udpActivityTimer;
    QDeadlineTimer _connectDeadline = QDeadlineTimer::Forever;
    GPSConnectionState _connection;
    int _source = -1;
    bool _sourceInstalled = false;
    QString _status;
#ifndef QGC_NO_SERIAL_LINK
    std::unique_ptr<QSerialPort> _serial;
    SerialPortManager::ReservationPtr _reservation;
    SerialPortManager::ReservationPtr _autoConnectExclusion;
    QString _serialDevice;
    qint32 _serialBaud = 0;
#endif
};
