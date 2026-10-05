#include "AtlasJetsonClient.h"

#include <algorithm>
#include <cmath>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonParseError>

AtlasJetsonClient::AtlasJetsonClient(QObject *parent)
    : QObject(parent)
{
    _reconnectTimer.setInterval(2000);
    _reconnectTimer.setSingleShot(true);

    connect(&_reconnectTimer, &QTimer::timeout,
            this, &AtlasJetsonClient::_connectNow);

    _heartbeatTimer.setInterval(kHeartbeatIntervalMs);
    _heartbeatTimer.setSingleShot(false);
    _heartbeatTimer.setTimerType(Qt::PreciseTimer);

    connect(&_heartbeatTimer, &QTimer::timeout,
            this, &AtlasJetsonClient::_onHeartbeat);

    connect(&_socket, &QTcpSocket::connected,
            this, &AtlasJetsonClient::_onConnected);

    connect(&_socket, &QTcpSocket::disconnected,
            this, &AtlasJetsonClient::_onDisconnected);

    connect(&_socket, &QTcpSocket::readyRead,
            this, &AtlasJetsonClient::_onReadyRead);

    connect(&_socket, &QTcpSocket::errorOccurred,
            this, &AtlasJetsonClient::_onSocketError);
}

AtlasJetsonClient::~AtlasJetsonClient()
{
    _wantConnection = false;
    _reconnectTimer.stop();
    _heartbeatTimer.stop();

    QObject::disconnect(&_socket, nullptr, this, nullptr);

    if (_socket.state() != QAbstractSocket::UnconnectedState) {
        _socket.abort();
    }
}

void AtlasJetsonClient::connectToHost(const QString &host, int port)
{
    const QString cleanHost = host.trimmed();

    if (cleanHost.isEmpty() || port <= 0 || port > 65535) {
        return;
    }

    const bool endpointHasChanged =
        (_host != cleanHost) ||
        (_port != static_cast<quint16>(port));

    _host = cleanHost;
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
    _clearRemoteLinkStatus();
}

void AtlasJetsonClient::_connectNow()
{
    if (!_wantConnection) {
        return;
    }

    if (_socket.state() == QAbstractSocket::ConnectedState ||
        _socket.state() == QAbstractSocket::ConnectingState) {
        return;
    }

    _rxBuffer.clear();

    qInfo() << "[ATLAS JETSON] Connecting to" << _host << _port;
    _socket.connectToHost(_host, _port);
}

void AtlasJetsonClient::_onConnected()
{
    _reconnectTimer.stop();
    _errorLogged = false;

    _socket.setSocketOption(QAbstractSocket::LowDelayOption, 1);
    _socket.setSocketOption(QAbstractSocket::KeepAliveOption, 1);

    // TCP aberto não significa serviço saudável. Só marcamos connected=true
    // quando chegar JSON válido do serviço ATLAS.
    _setConnected(false);
    _clearRemoteLinkStatus();

    _lastRxTimer.start();
    _heartbeatTimer.start();

    qInfo() << "[ATLAS JETSON] TCP connected to" << _host << _port;

    QJsonObject hello;
    hello.insert(QStringLiteral("type"), QStringLiteral("hello"));
    hello.insert(QStringLiteral("client"), QStringLiteral("ATLAS-QGroundControl"));
    hello.insert(QStringLiteral("protocol"), 2);
    hello.insert(QStringLiteral("role"), QStringLiteral("payload_gcs"));
    _sendCommand(hello);
}

void AtlasJetsonClient::_onDisconnected()
{
    _heartbeatTimer.stop();
    _lastRxTimer.invalidate();

    if (_connected) {
        qInfo() << "[ATLAS JETSON] Disconnected";
    }

    _setConnected(false);
    _clearRemoteLinkStatus();
    _scheduleReconnect();
}

void AtlasJetsonClient::_onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)

    if (!_errorLogged) {
        qWarning() << "[ATLAS JETSON] TCP error:" << _socket.errorString();
        _errorLogged = true;
    }

    _heartbeatTimer.stop();
    _setConnected(false);
    _clearRemoteLinkStatus();
    _scheduleReconnect();
}

void AtlasJetsonClient::_scheduleReconnect()
{
    if (_wantConnection && !_reconnectTimer.isActive()) {
        _reconnectTimer.start();
    }
}

void AtlasJetsonClient::_onHeartbeat()
{
    if (!_wantConnection || _socket.state() != QAbstractSocket::ConnectedState) {
        return;
    }

    if (_lastRxTimer.isValid() && _lastRxTimer.elapsed() > kWatchdogTimeoutMs) {
        qWarning() << "[ATLAS JETSON] Watchdog timeout - service not responding";

        _setConnected(false);
        _clearRemoteLinkStatus();
        _heartbeatTimer.stop();
        _socket.abort();
        _scheduleReconnect();
        return;
    }

    QJsonObject ping;
    ping.insert(QStringLiteral("type"), QStringLiteral("ping"));
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

void AtlasJetsonClient::_clearRemoteLinkStatus()
{
    _setHm30Connected(false);
    _setA8Connected(false);

    if (_rttMs != -1) {
        _rttMs = -1;
        emit statusChanged();
    }
}

void AtlasJetsonClient::_sendCommand(QJsonObject command)
{
    if (_socket.state() != QAbstractSocket::ConnectedState) {
        return;
    }

    command.insert(QStringLiteral("seq"), static_cast<double>(++_sequence));
    command.insert(QStringLiteral("ts_ms"),
                   static_cast<double>(QDateTime::currentMSecsSinceEpoch()));

    QByteArray packet = QJsonDocument(command).toJson(QJsonDocument::Compact);
    packet.append('\n');

    _socket.write(packet);
    _socket.flush();
}

void AtlasJetsonClient::gimbalRate(double yaw, double pitch)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("gimbal_rate"));
    command.insert(QStringLiteral("yaw"), std::clamp(yaw, -100.0, 100.0));
    command.insert(QStringLiteral("pitch"), std::clamp(pitch, -100.0, 100.0));
    _sendCommand(command);
}

void AtlasJetsonClient::gimbalStop()
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("gimbal_stop"));
    _sendCommand(command);
}

void AtlasJetsonClient::gimbalCenter()
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("gimbal_center"));
    _sendCommand(command);
}

void AtlasJetsonClient::gimbalPreset(const QString &preset)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("gimbal_preset"));
    command.insert(QStringLiteral("preset"), preset.trimmed().toUpper());
    _sendCommand(command);
}

void AtlasJetsonClient::zoom(int value)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("zoom"));
    command.insert(QStringLiteral("value"), std::clamp(value, -1, 1));
    _sendCommand(command);
}

void AtlasJetsonClient::takePhoto()
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("photo"));
    _sendCommand(command);
}

void AtlasJetsonClient::setRecording(bool enabled)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("record"));
    command.insert(QStringLiteral("enabled"), enabled);
    _sendCommand(command);
}

void AtlasJetsonClient::setAiEnabled(bool enabled)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("ai_enable"));
    command.insert(QStringLiteral("enabled"), enabled);
    _sendCommand(command);
}

void AtlasJetsonClient::setSahiEnabled(bool enabled)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("sahi_enable"));
    command.insert(QStringLiteral("enabled"), enabled);
    _sendCommand(command);
}

void AtlasJetsonClient::setDepthEnabled(bool enabled)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("depth_enable"));
    command.insert(QStringLiteral("enabled"), enabled);
    _sendCommand(command);
}

void AtlasJetsonClient::setAiModel(const QString &model)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("ai_model"));
    command.insert(QStringLiteral("model"), model.trimmed());
    _sendCommand(command);
}

void AtlasJetsonClient::setPerformanceProfile(const QString &profile)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("performance_profile"));
    command.insert(QStringLiteral("profile"), profile.trimmed().toUpper());
    _sendCommand(command);
}

void AtlasJetsonClient::setMissionProfile(const QString &profile)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("mission_profile"));
    command.insert(QStringLiteral("profile"), profile.trimmed().toUpper());
    _sendCommand(command);
}

void AtlasJetsonClient::setStreamMode(const QString &mode)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("stream_mode"));
    command.insert(QStringLiteral("mode"), mode.trimmed().toUpper());
    _sendCommand(command);
}

void AtlasJetsonClient::setVideoProfile(const QString &profile)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("video_profile"));
    command.insert(QStringLiteral("profile"), profile.trimmed().toUpper());
    _sendCommand(command);
}

void AtlasJetsonClient::setAiClasses(const QStringList &classes)
{
    if (!_connected) {
        return;
    }

    QJsonArray array;
    for (const QString &item : classes) {
        const QString clean = item.trimmed().toUpper();
        if (!clean.isEmpty()) {
            array.append(clean);
        }
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("ai_classes"));
    command.insert(QStringLiteral("classes"), array);
    _sendCommand(command);
}

void AtlasJetsonClient::setConfidence(double value)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("confidence"));
    command.insert(QStringLiteral("value"), std::clamp(value, 0.05, 0.99));
    _sendCommand(command);
}

void AtlasJetsonClient::setAiFpsTarget(int value)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("ai_fps_target"));
    command.insert(QStringLiteral("value"), std::clamp(value, 1, 60));
    _sendCommand(command);
}

void AtlasJetsonClient::setAiResolution(int value)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("ai_resolution"));
    command.insert(QStringLiteral("value"), std::clamp(value, 320, 1920));
    _sendCommand(command);
}

void AtlasJetsonClient::setSahiSlice(int value)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("sahi_slice"));
    command.insert(QStringLiteral("value"), std::clamp(value, 128, 1280));
    _sendCommand(command);
}

void AtlasJetsonClient::setTrackingModeEnabled(bool enabled)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("tracking_mode"));
    command.insert(QStringLiteral("enabled"), enabled);
    _sendCommand(command);
}

void AtlasJetsonClient::trackPoint(double xNorm, double yNorm)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("track_point"));
    command.insert(QStringLiteral("x"), std::clamp(xNorm, 0.0, 1.0));
    command.insert(QStringLiteral("y"), std::clamp(yNorm, 0.0, 1.0));
    _sendCommand(command);
}

void AtlasJetsonClient::trackRoi(double xNorm, double yNorm, double widthNorm, double heightNorm)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("track_roi"));
    command.insert(QStringLiteral("x"), std::clamp(xNorm, 0.0, 1.0));
    command.insert(QStringLiteral("y"), std::clamp(yNorm, 0.0, 1.0));
    command.insert(QStringLiteral("w"), std::clamp(widthNorm, 0.0, 1.0));
    command.insert(QStringLiteral("h"), std::clamp(heightNorm, 0.0, 1.0));
    _sendCommand(command);
}

void AtlasJetsonClient::setAutoGimbal(bool enabled)
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("auto_gimbal"));
    command.insert(QStringLiteral("enabled"), enabled);
    _sendCommand(command);
}

void AtlasJetsonClient::clearTracking()
{
    if (!_connected) {
        return;
    }

    QJsonObject command;
    command.insert(QStringLiteral("type"), QStringLiteral("tracking_clear"));
    _sendCommand(command);
}

bool AtlasJetsonClient::_applyStatus(const QJsonObject &message)
{
    bool changed = false;

    const auto assignBool = [&changed](bool &dst, const QJsonValue &value) {
        if (value.isBool()) {
            const bool v = value.toBool();
            if (dst != v) {
                dst = v;
                changed = true;
            }
        }
    };

    const auto assignInt = [&changed](int &dst, const QJsonValue &value) {
        if (value.isDouble()) {
            const int v = value.toInt();
            if (dst != v) {
                dst = v;
                changed = true;
            }
        }
    };

    const auto assignDouble = [&changed](double &dst, const QJsonValue &value) {
        if (value.isDouble()) {
            const double v = value.toDouble();
            if (!qFuzzyCompare(dst + 1.0, v + 1.0)) {
                dst = v;
                changed = true;
            }
        }
    };

    const auto assignString = [&changed](QString &dst, const QJsonValue &value) {
        if (value.isString()) {
            const QString v = value.toString();
            if (dst != v) {
                dst = v;
                changed = true;
            }
        }
    };

    if (message.contains(QStringLiteral("hm30_connected"))) {
        _setHm30Connected(message.value(QStringLiteral("hm30_connected")).toBool());
    }

    if (message.contains(QStringLiteral("a8_connected"))) {
        _setA8Connected(message.value(QStringLiteral("a8_connected")).toBool());
    }

    assignDouble(_jetsonCpuPercent, message.value(QStringLiteral("jetson_cpu_percent")));
    assignDouble(_jetsonGpuPercent, message.value(QStringLiteral("jetson_gpu_percent")));
    assignDouble(_jetsonRamUsedGB, message.value(QStringLiteral("jetson_ram_used_gb")));
    assignDouble(_jetsonRamTotalGB, message.value(QStringLiteral("jetson_ram_total_gb")));
    assignDouble(_jetsonTempC, message.value(QStringLiteral("jetson_temp_c")));
    assignDouble(_jetsonPowerW, message.value(QStringLiteral("jetson_power_w")));
    assignDouble(_jetsonSwapGB, message.value(QStringLiteral("jetson_swap_gb")));
    assignBool(_jetsonThrottled, message.value(QStringLiteral("jetson_throttled")));

    assignDouble(_hm30SignalPercent, message.value(QStringLiteral("hm30_signal_percent")));
    assignDouble(_hm30RssiDbm, message.value(QStringLiteral("hm30_rssi_dbm")));
    assignDouble(_hm30PacketLossPercent, message.value(QStringLiteral("hm30_packet_loss_percent")));
    assignDouble(_hm30LatencyMs, message.value(QStringLiteral("hm30_latency_ms")));
    assignDouble(_hm30UpMbps, message.value(QStringLiteral("hm30_up_mbps")));
    assignDouble(_hm30DownMbps, message.value(QStringLiteral("hm30_down_mbps")));

    assignBool(_recording, message.value(QStringLiteral("recording")));
    assignDouble(_zoomLevel, message.value(QStringLiteral("zoom_level")));
    assignDouble(_gimbalYawDeg, message.value(QStringLiteral("gimbal_yaw_deg")));
    assignDouble(_gimbalPitchDeg, message.value(QStringLiteral("gimbal_pitch_deg")));

    assignBool(_aiEnabled, message.value(QStringLiteral("ai_enabled")));
    assignBool(_sahiEnabled, message.value(QStringLiteral("sahi_enabled")));
    assignBool(_depthEnabled, message.value(QStringLiteral("depth_enabled")));
    assignBool(_tracking, message.value(QStringLiteral("tracking")));
    assignBool(_trackingModeEnabled, message.value(QStringLiteral("tracking_mode_enabled")));
    assignBool(_autoGimbal, message.value(QStringLiteral("auto_gimbal")));

    assignString(_aiModel, message.value(QStringLiteral("ai_model")));
    assignString(_performanceProfile, message.value(QStringLiteral("performance_profile")));
    assignString(_missionProfile, message.value(QStringLiteral("mission_profile")));
    assignString(_streamMode, message.value(QStringLiteral("stream_mode")));
    assignString(_videoProfile, message.value(QStringLiteral("video_profile")));

    if (message.value(QStringLiteral("ai_classes")).isArray()) {
        QStringList classes;
        const QJsonArray array = message.value(QStringLiteral("ai_classes")).toArray();
        for (const QJsonValue &value : array) {
            if (value.isString()) {
                classes.append(value.toString());
            }
        }
        if (_aiClasses != classes) {
            _aiClasses = classes;
            changed = true;
        }
    }

    assignDouble(_confidence, message.value(QStringLiteral("confidence")));
    assignInt(_aiFpsTarget, message.value(QStringLiteral("ai_fps_target")));
    assignInt(_aiResolution, message.value(QStringLiteral("ai_resolution")));
    assignInt(_sahiSlice, message.value(QStringLiteral("sahi_slice")));
    assignDouble(_aiFps, message.value(QStringLiteral("ai_fps")));
    assignDouble(_videoFps, message.value(QStringLiteral("video_fps")));
    assignDouble(_videoBitrateMbps, message.value(QStringLiteral("video_bitrate_mbps")));

    assignBool(_targetValid, message.value(QStringLiteral("target_valid")));
    assignInt(_targetId, message.value(QStringLiteral("target_id")));
    assignString(_targetClass, message.value(QStringLiteral("target_class")));
    assignDouble(_targetConfidence, message.value(QStringLiteral("target_confidence")));
    assignDouble(_targetDistanceM, message.value(QStringLiteral("target_distance_m")));
    assignDouble(_targetBearingDeg, message.value(QStringLiteral("target_bearing_deg")));
    assignDouble(_targetLatitude, message.value(QStringLiteral("target_latitude")));
    assignDouble(_targetLongitude, message.value(QStringLiteral("target_longitude")));

    return changed;
}

void AtlasJetsonClient::_onReadyRead()
{
    _rxBuffer.append(_socket.readAll());

    while (true) {
        const qsizetype newline = _rxBuffer.indexOf('\n');
        if (newline < 0) {
            break;
        }

        const QByteArray line = _rxBuffer.left(newline).trimmed();
        _rxBuffer.remove(0, newline + 1);

        if (line.isEmpty()) {
            continue;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);

        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            qWarning() << "[ATLAS JETSON] Invalid JSON:" << parseError.errorString();
            continue;
        }

        const QJsonObject message = document.object();
        _lastRxTimer.restart();

        if (!_connected) {
            qInfo() << "[ATLAS JETSON] Service responsive";
        }
        _setConnected(true);

        bool changed = _applyStatus(message);

        const QString type = message.value(QStringLiteral("type")).toString();

        if (type == QStringLiteral("pong") &&
            message.value(QStringLiteral("ping_ts_ms")).isDouble()) {
            const qint64 sentMs = static_cast<qint64>(
                message.value(QStringLiteral("ping_ts_ms")).toDouble());
            const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
            const int newRtt = static_cast<int>(std::clamp<qint64>(nowMs - sentMs, 0, 60000));
            if (_rttMs != newRtt) {
                _rttMs = newRtt;
                changed = true;
            }
        }

        if (type == QStringLiteral("event")) {
            const QString level = message.value(QStringLiteral("level")).toString(QStringLiteral("info"));
            const QString text = message.value(QStringLiteral("message")).toString();
            const qint64 timestampMs = message.value(QStringLiteral("ts_ms")).isDouble()
                ? static_cast<qint64>(message.value(QStringLiteral("ts_ms")).toDouble())
                : QDateTime::currentMSecsSinceEpoch();

            if (!text.isEmpty()) {
                emit eventReceived(level, text, timestampMs);
            }
        }

        if (changed) {
            emit statusChanged();
        }

        qInfo() << "[ATLAS JETSON] RX:"
                << QJsonDocument(message).toJson(QJsonDocument::Compact);
    }
}
