#include "AtlasJetsonClient.h"

#include <algorithm>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonParseError>

AtlasJetsonClient::AtlasJetsonClient(QObject *parent)
    : QObject(parent)
{
    // ---------------------------------------------------------------------
    // RECONEXÃO
    // ---------------------------------------------------------------------

    _reconnectTimer.setInterval(2000);
    _reconnectTimer.setSingleShot(true);

    connect(
        &_reconnectTimer,
        &QTimer::timeout,
        this,
        &AtlasJetsonClient::_connectNow
    );

    // ---------------------------------------------------------------------
    // HEARTBEAT
    // ---------------------------------------------------------------------

    _heartbeatTimer.setInterval(kHeartbeatIntervalMs);
    _heartbeatTimer.setSingleShot(false);
    _heartbeatTimer.setTimerType(Qt::PreciseTimer);

    connect(
        &_heartbeatTimer,
        &QTimer::timeout,
        this,
        &AtlasJetsonClient::_onHeartbeat
    );

    // ---------------------------------------------------------------------
    // TCP
    // ---------------------------------------------------------------------

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
    _wantConnection = false;

    _reconnectTimer.stop();
    _heartbeatTimer.stop();

    // Evita callbacks durante a destruição da aplicação.
    QObject::disconnect(
        &_socket,
        nullptr,
        this,
        nullptr
    );

    if (_socket.state() != QAbstractSocket::UnconnectedState) {
        _socket.abort();
    }
}


void AtlasJetsonClient::connectToHost(
    const QString &host,
    int port
)
{
    if (host.isEmpty()) {
        return;
    }

    if (port <= 0 || port > 65535) {
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
    _heartbeatTimer.stop();

    _lastRxTimer.invalidate();

    _socket.abort();

    _setConnected(false);
    _clearRemoteStatus();
}


void AtlasJetsonClient::_connectNow()
{
    if (!_wantConnection) {
        return;
    }

    if (
        _socket.state() == QAbstractSocket::ConnectedState ||
        _socket.state() == QAbstractSocket::ConnectingState
    ) {
        return;
    }

    _rxBuffer.clear();

    qInfo()
        << "[ATLAS JETSON] Connecting to"
        << _host
        << _port;

    _socket.connectToHost(
        _host,
        _port
    );
}


void AtlasJetsonClient::_onConnected()
{
    _reconnectTimer.stop();

    _errorLogged = false;

    _socket.setSocketOption(
        QAbstractSocket::LowDelayOption,
        1
    );

    _socket.setSocketOption(
        QAbstractSocket::KeepAliveOption,
        1
    );

    // O TCP está aberto, mas ainda não consideramos
    // a Jetson operacional até recebermos uma resposta.
    _setConnected(false);
    _clearRemoteStatus();

    _lastRxTimer.start();
    _heartbeatTimer.start();

    qInfo()
        << "[ATLAS JETSON] TCP connected to"
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
    _heartbeatTimer.stop();

    _lastRxTimer.invalidate();

    if (_connected) {
        qInfo()
            << "[ATLAS JETSON] Disconnected";
    }

    _setConnected(false);
    _clearRemoteStatus();

    _scheduleReconnect();
}


void AtlasJetsonClient::_onSocketError(
    QAbstractSocket::SocketError error
)
{
    Q_UNUSED(error)

    if (!_errorLogged) {
        qWarning()
            << "[ATLAS JETSON] TCP error:"
            << _socket.errorString();

        _errorLogged = true;
    }

    _heartbeatTimer.stop();

    _setConnected(false);
    _clearRemoteStatus();

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


void AtlasJetsonClient::_onHeartbeat()
{
    if (!_wantConnection) {
        return;
    }

    if (_socket.state() != QAbstractSocket::ConnectedState) {
        return;
    }

    // ---------------------------------------------------------------------
    // WATCHDOG
    //
    // O socket pode continuar tecnicamente aberto mesmo que o processo
    // remoto deixe de responder.
    // ---------------------------------------------------------------------

    if (
        _lastRxTimer.isValid() &&
        _lastRxTimer.elapsed() > kWatchdogTimeoutMs
    ) {
        qWarning()
            << "[ATLAS JETSON] Watchdog timeout - service not responding";

        _setConnected(false);
        _clearRemoteStatus();

        _heartbeatTimer.stop();

        // Forçamos uma nova sessão TCP.
        _socket.abort();

        _scheduleReconnect();

        return;
    }

    // ---------------------------------------------------------------------
    // PING
    // ---------------------------------------------------------------------

    QJsonObject ping;

    ping.insert(
        QStringLiteral("type"),
        QStringLiteral("ping")
    );

    _sendCommand(ping);
}


void AtlasJetsonClient::_setConnected(bool connected)
{
    if (_connected == connected) {
        return;
    }

    _connected = connected;

    emit connectedChanged();
}


void AtlasJetsonClient::_setHm30Connected(bool connected)
{
    if (_hm30Connected == connected) {
        return;
    }

    _hm30Connected = connected;

    emit hm30ConnectedChanged();
}


void AtlasJetsonClient::_setA8Connected(bool connected)
{
    if (_a8Connected == connected) {
        return;
    }

    _a8Connected = connected;

    emit a8ConnectedChanged();
}


void AtlasJetsonClient::_clearRemoteStatus()
{
    _setHm30Connected(false);
    _setA8Connected(false);
}


void AtlasJetsonClient::_sendCommand(QJsonObject command)
{
    // Importante:
    // usamos o estado REAL do socket e não _connected.
    //
    // Isto permite enviar heartbeat/hello antes de a Jetson
    // ser considerada operacional.
    if (_socket.state() != QAbstractSocket::ConnectedState) {
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
        QJsonDocument(command).toJson(
            QJsonDocument::Compact
        );

    packet.append('\n');

    _socket.write(packet);
    _socket.flush();
}


void AtlasJetsonClient::gimbalRate(
    double yaw,
    double pitch
)
{
    if (!_connected) {
        return;
    }

    yaw = std::clamp(
        yaw,
        -100.0,
        100.0
    );

    pitch = std::clamp(
        pitch,
        -100.0,
        100.0
    );

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
    if (!_connected) {
        return;
    }

    QJsonObject command;

    command.insert(
        QStringLiteral("type"),
        QStringLiteral("gimbal_stop")
    );

    _sendCommand(command);
}


void AtlasJetsonClient::gimbalCenter()
{
    if (!_connected) {
        return;
    }

    QJsonObject command;

    command.insert(
        QStringLiteral("type"),
        QStringLiteral("gimbal_center")
    );

    _sendCommand(command);
}


void AtlasJetsonClient::zoom(int value)
{
    if (!_connected) {
        return;
    }

    value = std::clamp(
        value,
        -1,
        1
    );

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
    if (!_connected) {
        return;
    }

    QJsonObject command;

    command.insert(
        QStringLiteral("type"),
        QStringLiteral("photo")
    );

    _sendCommand(command);
}


void AtlasJetsonClient::setRecording(bool enabled)
{
    if (!_connected) {
        return;
    }

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
    _rxBuffer.append(
        _socket.readAll()
    );

    while (true) {
        const qsizetype newline =
            _rxBuffer.indexOf('\n');

        if (newline < 0) {
            break;
        }

        const QByteArray line =
            _rxBuffer
                .left(newline)
                .trimmed();

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

        if (
            parseError.error !=
            QJsonParseError::NoError
        ) {
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

        // -------------------------------------------------------------
        // Qualquer mensagem JSON válida prova que o serviço remoto
        // está vivo.
        // -------------------------------------------------------------

        _lastRxTimer.restart();

        if (!_connected) {
            qInfo()
                << "[ATLAS JETSON] Service responsive";
        }

        _setConnected(true);

        // -------------------------------------------------------------
        // HM30
        // -------------------------------------------------------------

        if (
            message.contains(
                QStringLiteral("hm30_connected")
            )
        ) {
            _setHm30Connected(
                message.value(
                    QStringLiteral("hm30_connected")
                ).toBool()
            );
        }

        // -------------------------------------------------------------
        // A8
        // -------------------------------------------------------------

        if (
            message.contains(
                QStringLiteral("a8_connected")
            )
        ) {
            _setA8Connected(
                message.value(
                    QStringLiteral("a8_connected")
                ).toBool()
            );
        }

        qInfo()
            << "[ATLAS JETSON] RX:"
            << QJsonDocument(message)
                   .toJson(
                       QJsonDocument::Compact
                   );
    }
}
