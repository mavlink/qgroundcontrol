#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpSocket>

class AtlasJetsonClient : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool connected
               READ connected
               NOTIFY connectedChanged)

    Q_PROPERTY(bool hm30Connected
               READ hm30Connected
               NOTIFY hm30ConnectedChanged)

    Q_PROPERTY(bool a8Connected
               READ a8Connected
               NOTIFY a8ConnectedChanged)

    Q_PROPERTY(QString host
               READ host
               NOTIFY endpointChanged)

    Q_PROPERTY(int port
               READ port
               NOTIFY endpointChanged)

public:
    explicit AtlasJetsonClient(QObject *parent = nullptr);
    ~AtlasJetsonClient() override;

    bool connected() const
    {
        return _connected;
    }

    bool hm30Connected() const
    {
        return _hm30Connected;
    }

    bool a8Connected() const
    {
        return _a8Connected;
    }

    QString host() const
    {
        return _host;
    }

    int port() const
    {
        return static_cast<int>(_port);
    }

    Q_INVOKABLE void connectToHost(
        const QString &host,
        int port
    );

    Q_INVOKABLE void disconnectFromHost();

    Q_INVOKABLE void gimbalRate(
        double yaw,
        double pitch
    );

    Q_INVOKABLE void gimbalStop();
    Q_INVOKABLE void gimbalCenter();

    Q_INVOKABLE void zoom(int value);

    Q_INVOKABLE void takePhoto();

    Q_INVOKABLE void setRecording(
        bool enabled
    );

signals:
    void connectedChanged();
    void hm30ConnectedChanged();
    void a8ConnectedChanged();
    void endpointChanged();

private slots:
    void _onConnected();
    void _onDisconnected();
    void _onReadyRead();

    void _onSocketError(
        QAbstractSocket::SocketError error
    );

    void _connectNow();

private:
    void _setConnected(bool connected);
    void _setHm30Connected(bool connected);
    void _setA8Connected(bool connected);

    void _clearRemoteStatus();

    void _scheduleReconnect();

    void _sendCommand(
        QJsonObject command
    );

    QTcpSocket _socket;
    QTimer _reconnectTimer;

    QString _host =
        QStringLiteral("127.0.0.1");

    quint16 _port = 45900;

    QByteArray _rxBuffer;

    bool _connected = false;

    bool _hm30Connected = false;
    bool _a8Connected = false;

    bool _wantConnection = false;
    bool _errorLogged = false;

    quint64 _sequence = 0;
};
