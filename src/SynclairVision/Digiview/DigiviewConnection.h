#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>

#include "mavlink_types.h"

class DigiviewLegacyTcpTransport;

class DigiviewConnection : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(quint16 listenPort READ listenPort WRITE setListenPort NOTIFY listenPortChanged)
    Q_PROPERTY(quint16 legacyTcpControlPort READ legacyTcpControlPort WRITE setLegacyTcpControlPort NOTIFY
               legacyTcpControlPortChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    static constexpr const char* kDefaultHost = "192.168.4.60";
    static constexpr quint16 kDefaultPort = 14570;
    static constexpr quint16 kDefaultListenPort = 14571;
    static constexpr quint16 kDefaultLegacyTcpControlPort = 8555;

    explicit DigiviewConnection(QObject* parent = nullptr);

    QString host() const { return _host; }
    quint16 port() const { return _port; }
    quint16 listenPort() const { return _listenPort; }
    quint16 legacyTcpControlPort() const { return _legacyTcpControlPort; }
    bool connected() const { return _connected; }
    QString lastError() const { return _lastError; }

    void setHost(const QString& host);
    void setPort(quint16 port);
    void setListenPort(quint16 listenPort);
    void setLegacyTcpControlPort(quint16 port);

    bool connectToEndpoint();
    void disconnectFromEndpoint(bool preventAutomaticReconnect = false);
    bool sendMessage(const mavlink_message_t& message);

signals:
    void messageReceived(const mavlink_message_t& message);
    void hostChanged();
    void portChanged();
    void listenPortChanged();
    void legacyTcpControlPortChanged();
    void connectedChanged();
    void lastErrorChanged();
    void errorOccurred(const QString& error);

private slots:
    void _readPendingDatagrams();
    void _socketErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    void _setConnected(bool connected);
    void _setLastError(const QString& error);
    bool _resolveRemoteAddress(QHostAddress& remoteAddress);
    void _emitLegacyTcpHeartbeat();

    QUdpSocket _socket;
    DigiviewLegacyTcpTransport* _legacyTcpTransport = nullptr;
    QString _host = QStringLiteral("192.168.4.60");
    quint16 _port = kDefaultPort;
    quint16 _listenPort = kDefaultListenPort;
    quint16 _legacyTcpControlPort = kDefaultLegacyTcpControlPort;
    bool _connected = false;
    bool _automaticReconnectAllowed = true;
    bool _legacyTcpActive = false;
    QString _lastError;
    mavlink_status_t _mavlinkStatus {};
};
