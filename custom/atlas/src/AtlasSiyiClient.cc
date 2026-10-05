#include "AtlasSiyiClient.h"

#include <algorithm>
#include <cmath>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QtGlobal>

AtlasSiyiClient::AtlasSiyiClient(QObject *parent)
    : QObject(parent)
{
    _pollTimer.setInterval(kPollIntervalMs);
    _pollTimer.setSingleShot(false);
    _pollTimer.setTimerType(Qt::PreciseTimer);

    connect(&_pollTimer, &QTimer::timeout, this, &AtlasSiyiClient::_onPoll);
    connect(&_socket, &QUdpSocket::readyRead, this, &AtlasSiyiClient::_onReadyRead);
}

AtlasSiyiClient::~AtlasSiyiClient()
{
    _wantConnection = false;
    _pollTimer.stop();
    QObject::disconnect(&_socket, nullptr, this, nullptr);
    _socket.close();
}

void AtlasSiyiClient::connectToHost(const QString &host, int port)
{
    const QString cleanHost = host.trimmed();
    QHostAddress address;

    if (cleanHost.isEmpty() || !address.setAddress(cleanHost) || port <= 0 || port > 65535) {
        emit eventReceived(QStringLiteral("error"),
                           QStringLiteral("Invalid Direct A8 endpoint: %1:%2").arg(cleanHost).arg(port),
                           QDateTime::currentMSecsSinceEpoch());
        return;
    }

    const bool endpointHasChanged = (_host != cleanHost) || (_port != static_cast<quint16>(port));
    _host = cleanHost;
    _port = static_cast<quint16>(port);
    _address = address;

    if (endpointHasChanged) {
        emit endpointChanged();
    }

    if (_socket.state() == QAbstractSocket::UnconnectedState) {
        if (!_socket.bind(QHostAddress::AnyIPv4, 0,
                          QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
            emit eventReceived(QStringLiteral("error"),
                               QStringLiteral("Could not open SIYI UDP socket: %1").arg(_socket.errorString()),
                               QDateTime::currentMSecsSinceEpoch());
            return;
        }
    }

    _wantConnection = true;
    _pollCounter = 0;
    _lastRxTimer.invalidate();
    _setConnected(false);

    qInfo() << "[ATLAS SIYI] Direct A8 UDP target" << _host << _port;

    // Immediate probes. Ask recording/config first so the REC control is based
    // on camera-confirmed state as early as possible. Any valid SIYI response
    // marks the Direct A8 control route healthy.
    _sendCommand(0x0A); // config / recording state
    _sendCommand(0x0D); // gimbal attitude
    _sendCommand(0x18); // current zoom
    requestCodecSpecs(); // main + recording stream specs
    _sendCommand(0x01); // firmware version / liveness
    _pollTimer.start();
}

void AtlasSiyiClient::disconnectFromHost()
{
    _wantConnection = false;
    _pollTimer.stop();
    _lastRxTimer.invalidate();
    _setConnected(false);
}

void AtlasSiyiClient::_setConnected(bool connected)
{
    if (_connected == connected) {
        return;
    }

    _connected = connected;
    emit connectedChanged();
    emit statusChanged();
}

quint16 AtlasSiyiClient::_crc16Xmodem(const QByteArray &data)
{
    quint16 crc = 0x0000;
    for (const char raw : data) {
        crc ^= static_cast<quint16>(static_cast<quint8>(raw)) << 8;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000)
                ? static_cast<quint16>((crc << 1) ^ 0x1021)
                : static_cast<quint16>(crc << 1);
        }
    }
    return crc;
}

void AtlasSiyiClient::_appendInt16LE(QByteArray &data, qint16 value)
{
    const quint16 raw = static_cast<quint16>(value);
    data.append(static_cast<char>(raw & 0xFF));
    data.append(static_cast<char>((raw >> 8) & 0xFF));
}

void AtlasSiyiClient::_appendUInt16LE(QByteArray &data, quint16 value)
{
    data.append(static_cast<char>(value & 0xFF));
    data.append(static_cast<char>((value >> 8) & 0xFF));
}

qint16 AtlasSiyiClient::_readInt16LE(const QByteArray &data, int offset)
{
    if (offset < 0 || offset + 1 >= data.size()) {
        return 0;
    }
    const quint16 raw = static_cast<quint16>(static_cast<quint8>(data.at(offset))) |
                        (static_cast<quint16>(static_cast<quint8>(data.at(offset + 1))) << 8);
    return static_cast<qint16>(raw);
}

quint16 AtlasSiyiClient::_readUInt16LE(const QByteArray &data, int offset)
{
    if (offset < 0 || offset + 1 >= data.size()) {
        return 0;
    }
    return static_cast<quint16>(static_cast<quint8>(data.at(offset))) |
           (static_cast<quint16>(static_cast<quint8>(data.at(offset + 1))) << 8);
}

void AtlasSiyiClient::_sendCommand(quint8 commandId, const QByteArray &payload)
{
    if (!_wantConnection || _address.isNull()) {
        return;
    }

    QByteArray packet;
    packet.reserve(10 + payload.size());
    packet.append(static_cast<char>(0x55));
    packet.append(static_cast<char>(0x66));
    packet.append(static_cast<char>(0x01)); // need_ack

    const quint16 dataLength = static_cast<quint16>(payload.size());
    packet.append(static_cast<char>(dataLength & 0xFF));
    packet.append(static_cast<char>((dataLength >> 8) & 0xFF));

    const quint16 seq = _sequence++;
    packet.append(static_cast<char>(seq & 0xFF));
    packet.append(static_cast<char>((seq >> 8) & 0xFF));
    packet.append(static_cast<char>(commandId));
    packet.append(payload);

    const quint16 crc = _crc16Xmodem(packet);
    packet.append(static_cast<char>(crc & 0xFF));
    packet.append(static_cast<char>((crc >> 8) & 0xFF));

    if (_socket.writeDatagram(packet, _address, _port) != packet.size()) {
        qWarning() << "[ATLAS SIYI] UDP write failed:" << _socket.errorString();
    }
}

void AtlasSiyiClient::_onReadyRead()
{
    // Late UDP replies can arrive after the operator has switched away from
    // DIRECT A8. Drain them, but never resurrect an inactive backend.
    const bool acceptReplies = _wantConnection;

    while (_socket.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(_socket.pendingDatagramSize()));

        QHostAddress sender;
        quint16 senderPort = 0;
        const qint64 read = _socket.readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        Q_UNUSED(senderPort)
        if (read <= 0) {
            continue;
        }
        datagram.resize(static_cast<int>(read));

        if (!acceptReplies) {
            continue;
        }

        // The socket is ephemeral; reject anything not coming from the configured A8 IP.
        if (sender != _address) {
            continue;
        }

        int offset = 0;
        while (offset + 10 <= datagram.size()) {
            if (static_cast<quint8>(datagram.at(offset)) != 0x55 ||
                static_cast<quint8>(datagram.at(offset + 1)) != 0x66) {
                ++offset;
                continue;
            }

            const quint16 dataLength = static_cast<quint16>(static_cast<quint8>(datagram.at(offset + 3))) |
                                       (static_cast<quint16>(static_cast<quint8>(datagram.at(offset + 4))) << 8);
            const int frameSize = 8 + static_cast<int>(dataLength) + 2;
            if (offset + frameSize > datagram.size()) {
                break;
            }

            const QByteArray frame = datagram.mid(offset, frameSize);
            const quint16 expectedCrc = static_cast<quint16>(static_cast<quint8>(frame.at(frameSize - 2))) |
                                        (static_cast<quint16>(static_cast<quint8>(frame.at(frameSize - 1))) << 8);
            const quint16 actualCrc = _crc16Xmodem(frame.left(frameSize - 2));

            if (actualCrc == expectedCrc) {
                _lastRxTimer.restart();
                _setConnected(true);
                const quint8 commandId = static_cast<quint8>(frame.at(7));
                _handleFrame(commandId, frame.mid(8, dataLength));
            }

            offset += frameSize;
        }
    }
}

void AtlasSiyiClient::_handleFrame(quint8 commandId, const QByteArray &payload)
{
    bool changed = false;

    switch (commandId) {
    case 0x05: // manual zoom ACK, uint16 zoom * 10
        if (payload.size() >= 2) {
            const double value = static_cast<double>(_readUInt16LE(payload, 0)) / 10.0;
            if (value > 0.0 && !qFuzzyCompare(_zoomLevel + 1.0, value + 1.0)) {
                _zoomLevel = value;
                changed = true;
            }
        }
        break;

    case 0x0A: // gimbal configuration information
        if (payload.size() >= 4) {
            const quint8 recordState = static_cast<quint8>(payload.at(3));
            const bool recordingNow = (recordState == 1);
            if (_recording != recordingNow) {
                _recording = recordingNow;
                changed = true;
            }
            if (recordState == 2) {
                emit eventReceived(QStringLiteral("warning"),
                                   QStringLiteral("A8: TF card not detected"),
                                   QDateTime::currentMSecsSinceEpoch());
            } else if (recordState == 3) {
                emit eventReceived(QStringLiteral("warning"),
                                   QStringLiteral("A8: recording data loss reported by camera"),
                                   QDateTime::currentMSecsSinceEpoch());
            }
        }
        break;

    case 0x0B: // function feedback
        if (!payload.isEmpty()) {
            const quint8 info = static_cast<quint8>(payload.at(0));
            if (info == 1) {
                emit eventReceived(QStringLiteral("warning"),
                                   QStringLiteral("A8: photo failed (check TF card)"),
                                   QDateTime::currentMSecsSinceEpoch());
            } else if (info == 4) {
                emit eventReceived(QStringLiteral("warning"),
                                   QStringLiteral("A8: recording failed (check TF card)"),
                                   QDateTime::currentMSecsSinceEpoch());
            } else if (info == 5 && !_recording) {
                _recording = true;
                changed = true;
            } else if (info == 6 && _recording) {
                _recording = false;
                changed = true;
            }
        }
        break;

    case 0x0D: // yaw, pitch, roll, yaw vel, pitch vel, roll vel; 0.1 deg units
        if (payload.size() >= 6) {
            const double yaw = static_cast<double>(_readInt16LE(payload, 0)) / 10.0;
            const double pitch = static_cast<double>(_readInt16LE(payload, 2)) / 10.0;
            const double roll = static_cast<double>(_readInt16LE(payload, 4)) / 10.0;
            if (!qFuzzyCompare(_gimbalYawDeg + 1000.0, yaw + 1000.0) ||
                !qFuzzyCompare(_gimbalPitchDeg + 1000.0, pitch + 1000.0) ||
                !qFuzzyCompare(_gimbalRollDeg + 1000.0, roll + 1000.0)) {
                _gimbalYawDeg = yaw;
                _gimbalPitchDeg = pitch;
                _gimbalRollDeg = roll;
                changed = true;
            }
        }
        break;

    case 0x0E: // absolute angle control ACK
        if (payload.size() >= 6) {
            _gimbalYawDeg = static_cast<double>(_readInt16LE(payload, 0)) / 10.0;
            _gimbalPitchDeg = static_cast<double>(_readInt16LE(payload, 2)) / 10.0;
            _gimbalRollDeg = static_cast<double>(_readInt16LE(payload, 4)) / 10.0;
            changed = true;
        }
        break;

    case 0x18: // current zoom, integer byte + decimal byte
        if (payload.size() >= 2) {
            const double value = static_cast<double>(static_cast<quint8>(payload.at(0))) +
                                 static_cast<double>(static_cast<quint8>(payload.at(1))) / 10.0;
            if (value > 0.0 && !qFuzzyCompare(_zoomLevel + 1.0, value + 1.0)) {
                _zoomLevel = value;
                changed = true;
            }
        }
        break;

    case 0x20: // codec specs: stream, codec, width, height, bitrate Kbps, fps
        if (payload.size() >= 9) {
            const int streamType = static_cast<quint8>(payload.at(0));
            const int codecValue = static_cast<quint8>(payload.at(1));
            const QString codec = codecValue == 1 ? QStringLiteral("H264")
                                   : codecValue == 2 ? QStringLiteral("H265")
                                                     : QStringLiteral("UNKNOWN");
            const int width = _readUInt16LE(payload, 2);
            const int height = _readUInt16LE(payload, 4);
            const int bitrate = _readUInt16LE(payload, 6);
            const int fps = static_cast<quint8>(payload.at(8));

            if (streamType == 1) {
                _mainStreamInfoValid = true;
                _mainStreamCodec = codec;
                _mainStreamWidth = width;
                _mainStreamHeight = height;
                _mainStreamBitrateKbps = bitrate;
                _mainStreamFps = fps;
                changed = true;
            } else if (streamType == 0) {
                _recordStreamInfoValid = true;
                _recordStreamCodec = codec;
                _recordStreamWidth = width;
                _recordStreamHeight = height;
                _recordStreamBitrateKbps = bitrate;
                _recordStreamFps = fps;
                changed = true;
            }
        }
        break;

    case 0x21: // set codec specs ACK: stream_type, status
        if (payload.size() >= 2) {
            const int streamType = static_cast<quint8>(payload.at(0));
            const bool ok = static_cast<quint8>(payload.at(1)) == 1;
            emit eventReceived(ok ? QStringLiteral("info") : QStringLiteral("warning"),
                               QStringLiteral("A8 %1 stream codec settings: %2")
                                   .arg(streamType == 0 ? QStringLiteral("record") : QStringLiteral("main"))
                                   .arg(ok ? QStringLiteral("applied") : QStringLiteral("rejected")),
                               QDateTime::currentMSecsSinceEpoch());
            if (ok) {
                QTimer::singleShot(250, this, [this, streamType]() {
                    if (!_wantConnection) return;
                    QByteArray request;
                    request.append(static_cast<char>(streamType));
                    _sendCommand(0x20, request);
                });
            }
        }
        break;

    default:
        break;
    }

    if (changed) {
        emit statusChanged();
    }
}

void AtlasSiyiClient::_checkWatchdog()
{
    if (_wantConnection && _lastRxTimer.isValid() && _lastRxTimer.elapsed() > kWatchdogTimeoutMs) {
        _setConnected(false);
    }
}

void AtlasSiyiClient::_onPoll()
{
    if (!_wantConnection) {
        return;
    }

    _checkWatchdog();
    ++_pollCounter;

    _sendCommand(0x0D); // attitude ~2.9 Hz
    if ((_pollCounter % 3) == 0) {
        _sendCommand(0x0A); // record/config ~1 Hz
    }
    if ((_pollCounter % 5) == 0) {
        _sendCommand(0x18); // zoom ~0.6 Hz
    }
    if ((_pollCounter % 20) == 0) {
        _sendCommand(0x01); // low-rate liveness probe
    }
}

void AtlasSiyiClient::gimbalRate(double yaw, double pitch)
{
    if (!_wantConnection) {
        return;
    }

    const qint8 yawCmd = static_cast<qint8>(std::lround(std::clamp(yaw, -100.0, 100.0)));
    const qint8 pitchCmd = static_cast<qint8>(std::lround(std::clamp(pitch, -100.0, 100.0)));

    QByteArray payload;
    payload.append(static_cast<char>(yawCmd));
    payload.append(static_cast<char>(pitchCmd));
    _sendCommand(0x07, payload);
}

void AtlasSiyiClient::gimbalStop()
{
    gimbalRate(0.0, 0.0);
}

void AtlasSiyiClient::gimbalCenter()
{
    QByteArray payload;
    payload.append(static_cast<char>(0x01));
    _sendCommand(0x08, payload);
}

void AtlasSiyiClient::gimbalPreset(const QString &preset)
{
    const QString key = preset.trimmed().toUpper();
    if (key == QStringLiteral("FORWARD") || key == QStringLiteral("FWD") || key == QStringLiteral("CENTER")) {
        gimbalCenter();
        return;
    }

    double yaw = 0.0;
    double pitch = 0.0;
    if (key == QStringLiteral("DOWN")) {
        pitch = -90.0;
    } else if (key == QStringLiteral("LEFT")) {
        yaw = -90.0;
    } else if (key == QStringLiteral("RIGHT")) {
        yaw = 90.0;
    } else {
        return; // TARGET is a Jetson tracking feature
    }

    QByteArray payload;
    _appendInt16LE(payload, static_cast<qint16>(std::lround(yaw * 10.0)));
    _appendInt16LE(payload, static_cast<qint16>(std::lround(pitch * 10.0)));
    _sendCommand(0x0E, payload);
}

void AtlasSiyiClient::zoom(int value)
{
    QByteArray payload;
    payload.append(static_cast<char>(static_cast<qint8>(std::clamp(value, -1, 1))));
    _sendCommand(0x05, payload);
}

void AtlasSiyiClient::takePhoto()
{
    QByteArray payload;
    payload.append(static_cast<char>(0x00));
    _sendCommand(0x0C, payload);
}

void AtlasSiyiClient::setRecording(bool enabled)
{
    if (_connected && _recording == enabled) {
        return;
    }

    // SIYI 0x0C/0x02 toggles recording. The UI requests the opposite of the
    // last state confirmed by 0x0A, then we immediately re-query the camera.
    QByteArray payload;
    payload.append(static_cast<char>(0x02));
    _sendCommand(0x0C, payload);

    QTimer::singleShot(180, this, [this]() {
        if (_wantConnection) {
            _sendCommand(0x0A);
        }
    });
}


void AtlasSiyiClient::requestCodecSpecs()
{
    if (!_wantConnection) {
        return;
    }

    QByteArray recordRequest;
    recordRequest.append(static_cast<char>(0));
    _sendCommand(0x20, recordRequest);

    QByteArray mainRequest;
    mainRequest.append(static_cast<char>(1));
    _sendCommand(0x20, mainRequest);
}

static bool atlasValidateSiyiCodecSettings(const QString &codec, int width, int height, int bitrateKbps, quint8 *codecValue)
{
    const QString c = codec.trimmed().toUpper();
    if (c == QStringLiteral("H264")) {
        *codecValue = 1;
    } else if (c == QStringLiteral("H265")) {
        *codecValue = 2;
    } else {
        return false;
    }

    // A8 External SDK v1.6 explicitly documents these writable resolutions.
    // UniGCS exposes higher recording resolutions too, but v0.12.2 deliberately
    // does not send undocumented A8 2K/4K payloads.
    const bool resolutionOk = (width == 1920 && height == 1080) ||
                              (width == 1280 && height == 720);
    return resolutionOk && bitrateKbps >= 500 && bitrateKbps <= 30000;
}

void AtlasSiyiClient::configureMainStream(const QString &codec, int width, int height, int bitrateKbps)
{
    quint8 codecValue = 0;
    if (!_wantConnection || !atlasValidateSiyiCodecSettings(codec, width, height, bitrateKbps, &codecValue)) {
        emit eventReceived(QStringLiteral("warning"),
                           QStringLiteral("A8 main-stream settings rejected locally (supported: H264/H265, 720p/1080p, 0.5-30 Mbps)"),
                           QDateTime::currentMSecsSinceEpoch());
        return;
    }

    QByteArray payload;
    payload.append(static_cast<char>(1)); // main stream
    payload.append(static_cast<char>(codecValue));
    _appendUInt16LE(payload, static_cast<quint16>(width));
    _appendUInt16LE(payload, static_cast<quint16>(height));
    _appendUInt16LE(payload, static_cast<quint16>(bitrateKbps));
    payload.append(static_cast<char>(0)); // reserved; FPS is not writable in documented A8 SDK
    _sendCommand(0x21, payload);
}

void AtlasSiyiClient::configureRecordStream(int width, int height, int bitrateKbps)
{
    if (!_wantConnection) {
        return;
    }

    // A8 External SDK V0.1.1 marks recording-stream codec changes as not
    // supported. The 0x21 packet still contains VideoEncType, so preserve the
    // codec reported by 0x20 and only change the documented resolution/bitrate.
    if (!_recordStreamInfoValid || (_recordStreamCodec != QStringLiteral("H264") &&
                                    _recordStreamCodec != QStringLiteral("H265"))) {
        emit eventReceived(QStringLiteral("warning"),
                           QStringLiteral("Read A8 recording-stream status first; its codec must be preserved"),
                           QDateTime::currentMSecsSinceEpoch());
        return;
    }

    const bool resolutionOk = (width == 1920 && height == 1080) ||
                              (width == 1280 && height == 720);
    if (!resolutionOk || bitrateKbps < 500 || bitrateKbps > 30000) {
        emit eventReceived(QStringLiteral("warning"),
                           QStringLiteral("A8 record-stream settings rejected locally (safe SDK subset: 720p/1080p, 0.5-30 Mbps; codec preserved)"),
                           QDateTime::currentMSecsSinceEpoch());
        return;
    }

    const quint8 codecValue = (_recordStreamCodec == QStringLiteral("H264")) ? 1 : 2;

    QByteArray payload;
    payload.append(static_cast<char>(0)); // recording stream
    payload.append(static_cast<char>(codecValue)); // preserve codec; SDK says recording codec is not writable
    _appendUInt16LE(payload, static_cast<quint16>(width));
    _appendUInt16LE(payload, static_cast<quint16>(height));
    _appendUInt16LE(payload, static_cast<quint16>(bitrateKbps));
    payload.append(static_cast<char>(0)); // reserved; FPS is not writable in documented A8 SDK
    _sendCommand(0x21, payload);
}

void AtlasSiyiClient::setGimbalWorkingMode(const QString &mode)
{
    if (!_wantConnection) return;
    const QString key = mode.trimmed().toUpper();
    quint8 function = 0;
    if (key == QStringLiteral("LOCK")) function = 3;
    else if (key == QStringLiteral("FOLLOW")) function = 4;
    else if (key == QStringLiteral("FPV")) function = 5;
    else return;

    QByteArray payload;
    payload.append(static_cast<char>(function));
    _sendCommand(0x0C, payload);
}

void AtlasSiyiClient::setVideoOutput(const QString &output)
{
    if (!_wantConnection) return;
    const QString key = output.trimmed().toUpper();
    quint8 function = 0;
    if (key == QStringLiteral("HDMI")) function = 6;
    else if (key == QStringLiteral("CVBS")) function = 7;
    else if (key == QStringLiteral("ETHERNET") || key == QStringLiteral("OFF")) function = 8;
    else return;

    QByteArray payload;
    payload.append(static_cast<char>(function));
    _sendCommand(0x0C, payload);
    emit eventReceived(QStringLiteral("info"),
                       QStringLiteral("A8 video output set to %1; camera restart may be required").arg(key),
                       QDateTime::currentMSecsSinceEpoch());
}
