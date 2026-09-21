#pragma once

#include <memory>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtQmlIntegration/QtQmlIntegration>

#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
class QSerialPort;
#endif

class AutoConnectSettings;
class QGCPositionManager;
class UdpIODevice;

/// Owns the NMEA input device; PositionManager owns decoding and GCS fix state.
/// Lifecycle notifications may replace or delete this owner; retiring devices survive decoder detachment.
class NMEASourceManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by GPSManager")
    Q_PROPERTY(ConnectionState connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString connectionStatusText READ connectionStatusText NOTIFY connectionStateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY connectionStateChanged)
    friend class NMEASourceManagerTest;

public:
    enum class ConnectionState
    {
        Disabled,
        WaitingForDevice,
        Connected,
        Error,
    };
    Q_ENUM(ConnectionState)

    NMEASourceManager(AutoConnectSettings* settings, QGCPositionManager* positionManager, QObject* parent = nullptr);
    ~NMEASourceManager() override;
    void update();
    void stop();

    ConnectionState connectionState() const { return _connectionState; }

    QString connectionStatusText() const;

    QString errorMessage() const { return _errorMessage; }

signals:
    void connectionStateChanged();

private:
    void _stop(const char* reason, bool resetStatus = true);
    void _setConnectionState(ConnectionState state, const QString& error = {});

    QPointer<AutoConnectSettings> _settings;
    QPointer<QGCPositionManager> _positionManager;
    std::unique_ptr<UdpIODevice> _udp;
    int _source = -1;
    bool _sourceInstalled = false;
    quint64 _revision = 0;
    bool _destroying = false;
    ConnectionState _connectionState = ConnectionState::Disabled;
    QString _errorMessage;
#ifndef QGC_NO_SERIAL_LINK
    void _updateSerialRouting();
    std::unique_ptr<QSerialPort> _serial;
    SerialPortManager::ReservationPtr _reservation;
    SerialPortManager::ReservationPtr _autoConnectExclusion;
    QString _serialDevice;
    qint32 _serialBaud = 0;
#endif
};
