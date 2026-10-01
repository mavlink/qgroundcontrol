#include "AtlasJetsonClient.h"

#include <algorithm>

#include <QtCore/QDateTime>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonParseError>
#include <QtCore/QDebug>

AtlasJetsonClient::AtlasJetsonClient(QObject *parent)
    : QObject(parent)
{
    _reconnectTimer.setInterval(2000);
    _reconnectTimer.setSingleShot(true);

    connect(
        &_reconnectTimer,
        &QTimer::timeout,
        this,
        &AtlasJetsonClient::_connectNow
    );

    connect(
        &_socket,
        &QTcpSocket::connected,
        this,
        &AtlasJetsonClient::_onConnected
    );

    connect(
        &_socket,
        &QTcpSocket::disconnected,
        this,
        &AtlasJetsonClient::_onDisconnected
    );

    connect(
        &_socket,
        &QTcpSocket::readyRead,
        this,
        &AtlasJetsonClient::_onReadyRead
    );

    connect(
        &_socket,
        &QTcpSocket::errorOccurred,
        this,
        &AtlasJetsonClient::_onSocketError
    );
}

AtlasJetsonClient::~AtlasJetsonClient()
{
    // A aplicação está a fechar.
    // Impede qualquer tentativa de reconexão.
    _wantConnection = false;

    // Cancela um eventual reconnect pendente.
    _reconnectTimer.stop();

    // Fecha imediatamente a ligação TCP, se ainda existir.
    if (_socket.state() != QAbstractSocket::UnconnectedState) {
        _socket.abort();
    }
}

void AtlasJetsonClient::connectToHost(const QString &host, int port)
{
    if (host.isEmpty()) {
        return;
    }

    if ((port <= 0) || (port > 65535)) {
        return;
    }

    const bool endpointHasChanged =
        (_host != host) ||
        (_port != static_cast<quint16>(port));

    _host = host;
    _port = static_cast<quint16>(port);

    if (endpointHasChanged) {
        emit endpointChanged();
    }

    _wantConnection = true;
    _reconnectTimer.stop();

    if (_socket.state() != QAbstractSocket::UnconnectedState) {
        _socket.abort();
    }

    _connectNow();
}

void AtlasJetsonClient::disconnectFromHost()
{
    _wantConnection = false;

    _reconnectTimer.stop();

    _socket.abort();

    _setConnected(false);
}

void AtlasJetsonClient::_connectNow()
{
    if (!_wantConnection) {
        return;
    }

    if ((_socket.state() == QAbstractSocket::ConnectedState) ||
        (_socket.state() == QAbstractSocket::ConnectingState)) {
        return;
    }

    qInfo()
        << "[ATLAS JETSON] Connecting to"
        << _host
        << _port;

    _socket.connectToHost(_host, _port);
}

void AtlasJetsonClient::_onConnected()
{
    _errorLogged = false;

    _socket.setSocketOption(
        QAbstractSocket::LowDelayOption,
        1
    );

    _socket.setSocketOption(
        QAbstractSocket::KeepAliveOption,
        1
    );

    _setConnected(true);

    qInfo()
        << "[ATLAS JETSON] Connected to"
        << _host
        << _port;

    QJsonObject hello;

    hello.insert(
        QStringLiteral("type"),
        QStringLiteral("hello")
    );

    hello.insert(
        QStringLiteral("client"),
        QStringLiteral("ATLAS-QGroundControl")
    );

    hello.insert(
        QStringLiteral("protocol"),
        1
    );

    _sendCommand(hello);
}

void AtlasJetsonClient::_onDisconnected()
{
    if (_connected) {
        qInfo() << "[ATLAS JETSON] Disconnected";
    }

    _setConnected(false);

    _scheduleReconnect();
}

void AtlasJetsonClient::_onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)

    if (!_errorLogged) {
        qWarning()
            << "[ATLAS JETSON] TCP error:"
            << _socket.errorString();

        _errorLogged = true;
    }

    _setConnected(false);

    _scheduleReconnect();
}

void AtlasJetsonClient::_scheduleReconnect()
{
    if (!_wantConnection) {
        return;
    }

    if (!_reconnectTimer.isActive()) {
        _reconnectTimer.start();
    }
}

void AtlasJetsonClient::_setConnected(bool connected)
{
    if (_connected == connected) {
        return;
    }

    _connected = connected;

    emit connectedChanged();
}

void AtlasJetsonClient::_sendCommand(QJsonObject command)
{
    if (!_connected) {
        return;
    }

    command.insert(
        QStringLiteral("seq"),
        static_cast<double>(++_sequence)
    );

    command.insert(
        QStringLiteral("ts_ms"),
        static_cast<double>(
            QDateTime::currentMSecsSinceEpoch()
        )
    );

    QByteArray packet =
        QJsonDocument(command)
            .toJson(QJsonDocument::Compact);

    packet.append('\n');

    _socket.write(packet);

    // Tenta empurrar imediatamente os dados para TCP.
    // LowDelayOption desativa Nagle.
    _socket.flush();
}

void AtlasJetsonClient::gimbalRate(double yaw, double pitch)
{
    yaw = std::clamp(yaw, -100.0, 100.0);
    pitch = std::clamp(pitch, -100.0, 100.0);

    QJsonObject command;

    command.insert(
        QStringLiteral("type"),
        QStringLiteral("gimbal_rate")
    );

    command.insert(
        QStringLiteral("yaw"),
        yaw
    );

    command.insert(
        QStringLiteral("pitch"),
        pitch
    );

    _sendCommand(command);
}

void AtlasJetsonClient::gimbalStop()
{
    QJsonObject command;

    command.insert(
        QStringLiteral("type"),
        QStringLiteral("gimbal_stop")
    );

    _sendCommand(command);
}

void AtlasJetsonClient::gimbalCenter()
{
    QJsonObject command;

    command.insert(
        QStringLiteral("type"),
        QStringLiteral("gimbal_center")
    );

    _sendCommand(command);
}

void AtlasJetsonClient::zoom(int value)
{
    value = std::clamp(value, -1, 1);

    QJsonObject command;

    command.insert(
        QStringLiteral("type"),
        QStringLiteral("zoom")
    );

    command.insert(
        QStringLiteral("value"),
        value
    );

    _sendCommand(command);
}

void AtlasJetsonClient::takePhoto()
{
    QJsonObject command;

    command.insert(
        QStringLiteral("type"),
        QStringLiteral("photo")
    );

    _sendCommand(command);
}

void AtlasJetsonClient::setRecording(bool enabled)
{
    QJsonObject command;

    command.insert(
        QStringLiteral("type"),
        QStringLiteral("record")
    );

    command.insert(
        QStringLiteral("enabled"),
        enabled
    );

    _sendCommand(command);
}

void AtlasJetsonClient::_onReadyRead()
{
    _rxBuffer.append(_socket.readAll());

    while (true) {
        const qsizetype newline =
            _rxBuffer.indexOf('\n');

        if (newline < 0) {
            break;
        }

        const QByteArray line =
            _rxBuffer.left(newline).trimmed();

        _rxBuffer.remove(
            0,
            newline + 1
        );

        if (line.isEmpty()) {
            continue;
        }

        QJsonParseError parseError;

        const QJsonDocument document =
            QJsonDocument::fromJson(
                line,
                &parseError
            );

        if (parseError.error != QJsonParseError::NoError) {
            qWarning()
                << "[ATLAS JETSON] Invalid JSON:"
                << parseError.errorString();

            continue;
        }

        if (!document.isObject()) {
            continue;
        }

        const QJsonObject message =
            document.object();

        qInfo()
            << "[ATLAS JETSON] RX:"
            << QJsonDocument(message)
                   .toJson(QJsonDocument::Compact);
    }
}
