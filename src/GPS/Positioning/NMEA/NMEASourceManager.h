#pragma once

#include <memory>

#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSNotificationQueue.h"
#include "GPSPositionSourceRegistration.h"

#ifndef QGC_NO_SERIAL_LINK
#include "SerialPortManager.h"
class QSerialPort;
#endif

class AutoConnectSettings;
class QGCPositionManager;
class UdpIODevice;
class QTcpSocket;
class QIODevice;
class NMEADecoderSession;
class GPSSourceHealth;

/// Owns the input, decoder, and position registration as one retiring session.
/// Lifecycle notifications may replace or delete this owner; retiring devices survive decoder detachment.
class NMEASourceManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by GPSManager")
    Q_PROPERTY(ConnectionState connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString connectionStatusText READ connectionStatusText NOTIFY connectionStateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY connectionStateChanged)
    Q_PROPERTY(GPSSourceHealth* health READ health NOTIFY sourceChanged)
    Q_PROPERTY(bool receiving READ receiving NOTIFY activityChanged)
    Q_PROPERTY(bool hasData READ hasData NOTIFY activityChanged)
    Q_MOC_INCLUDE("GPSSourceHealth.h")
    friend class NMEASourceManagerTest;
    friend class PositionManagerTest;

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

    GPSSourceHealth* health() const;
    bool receiving() const;
    bool hasData() const;

signals:
    void connectionStateChanged();
    void sourceChanged();
    void activityChanged();

private:
    void _stop(const char* reason, bool resetStatus = true);
    void _setConnectionState(ConnectionState state, const QString& error = {});
    void _startDecoder(QIODevice* device);
    void _retireDecoder(const char* reason);

    static constexpr int kTcpConnectTimeoutMs = 10000;

    QPointer<AutoConnectSettings> _settings;
    QPointer<QGCPositionManager> _positionManager;

    struct DecoderBinding
    {
        QPointer<QIODevice> device;
        std::unique_ptr<NMEADecoderSession> decoder;
        GPSPositionSourceRegistration registration;
        QMetaObject::Connection closedConnection;
        QMetaObject::Connection destroyedConnection;
    };

    struct InputSession
    {
        int source = -1;
#ifndef QGC_NO_SERIAL_LINK
        SerialPortManager::ReservationPtr reservation;
        std::unique_ptr<QSerialPort> serial;
        QString serialDevice;
        qint32 serialBaud = 0;
#endif
        std::unique_ptr<UdpIODevice> udp;
        std::unique_ptr<QTcpSocket> tcp;
        QString tcpHost;
        quint16 tcpPort = 0;
        QElapsedTimer tcpConnecting;
        DecoderBinding binding;
    } _input;
    quint64 _revision = 0;
    quint64 _decoderGeneration = 0;
    bool _destroying = false;
    GPSNotificationQueue _notifications{this};
    ConnectionState _connectionState = ConnectionState::Disabled;
    QString _errorMessage;
#ifndef QGC_NO_SERIAL_LINK
    void _updateSerialRouting();
    SerialPortManager::ReservationPtr _autoConnectExclusion;
#endif
};
