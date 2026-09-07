#include "DigiviewConnection.h"

#include "DigiviewLegacyTcpTransport.h"
#include "MAVLinkProtocol.h"
#include "QGCLoggingCategory.h"
#include "sv_mavlink_dialect/sv_mavlink_dialect.h"

#include <QtCore/QByteArray>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtNetwork/QHostInfo>

#include <array>

QGC_LOGGING_CATEGORY(DigiviewConnectionLog, "Digiview.Connection")

namespace {

constexpr char kSynclairSettingsGroup[] = "SynclairVisionSettings";
constexpr char kLegacyTcpControlSetting[] = "networkForceRtspVideoOverTcp";
constexpr uint8_t kDigiviewSystemId = 252;
constexpr uint8_t kDigiviewComponentId = 66;
constexpr quint16 kDigiviewRouterPort = 14570;
constexpr int kRestartHeartbeatLossTimeoutMs = 1500;

bool legacyTcpControlEnabled()
{
    QSettings settings;
    settings.beginGroup(QLatin1String(kSynclairSettingsGroup));
    return settings.value(QLatin1String(kLegacyTcpControlSetting), false).toBool();
}

} // namespace

DigiviewConnection::DigiviewConnection(QObject* parent)
    : QObject(parent)
    , _legacyTcpTransport(new DigiviewLegacyTcpTransport(this))
{
    connect(&_socket, &QUdpSocket::readyRead, this, &DigiviewConnection::_readPendingDatagrams);
    connect(&_socket, &QUdpSocket::errorOccurred, this, &DigiviewConnection::_socketErrorOccurred);
    connect(_legacyTcpTransport, &DigiviewLegacyTcpTransport::connectedToEndpoint, this, [this] {
        _setLastError(QString());
        _setConnected(true);
        QTimer::singleShot(0, this, &DigiviewConnection::_emitLegacyTcpHeartbeat);
    });
    connect(_legacyTcpTransport, &DigiviewLegacyTcpTransport::disconnectedFromEndpoint, this, [this] {
        if (_legacyTcpActive) {
            _setConnected(false);
            if (_restartObservationGeneration) {
                emit restartTransportDownObserved(*_restartObservationGeneration);
            }
        }
    });
    connect(_legacyTcpTransport, &DigiviewLegacyTcpTransport::messageReceived,
            this, &DigiviewConnection::messageReceived);
    connect(_legacyTcpTransport, &DigiviewLegacyTcpTransport::errorOccurred, this, [this](const QString& error) {
        if (_legacyTcpActive) {
            _setLastError(error);
            if (!_legacyTcpTransport->connected()) {
                _setConnected(false);
            }
        }
    });
    connect(_legacyTcpTransport, &DigiviewLegacyTcpTransport::restartQuitSent,
            this, [this](quint64 generation) {
                emit restartQuitSent(generation);
            });
    connect(_legacyTcpTransport, &DigiviewLegacyTcpTransport::restartFailed,
            this, &DigiviewConnection::restartFailed);
    connect(&_restartHeartbeatLossTimer, &QTimer::timeout,
            this, &DigiviewConnection::_restartHeartbeatLossTimeout);
    _restartHeartbeatLossTimer.setSingleShot(true);
}

void DigiviewConnection::setHost(const QString& host)
{
    const QString trimmedHost = host.trimmed();
    if (trimmedHost == _host) {
        return;
    }

    if (_legacyTcpActive) {
        _legacyTcpTransport->disconnectFromEndpoint();
    }
    _host = trimmedHost;
    emit hostChanged();
}

void DigiviewConnection::setPort(quint16 port)
{
    if (port == _port) {
        return;
    }

    if (_legacyTcpActive) {
        _legacyTcpTransport->disconnectFromEndpoint();
    }
    _port = port;
    emit portChanged();
}

void DigiviewConnection::setListenPort(quint16 listenPort)
{
    if (listenPort == _listenPort) {
        return;
    }

    if (_legacyTcpActive) {
        _legacyTcpTransport->disconnectFromEndpoint();
    }
    _listenPort = listenPort;
    emit listenPortChanged();
}

void DigiviewConnection::setLegacyTcpControlPort(quint16 port)
{
    if (port == _legacyTcpControlPort) {
        return;
    }

    if (_legacyTcpActive) {
        _legacyTcpTransport->disconnectFromEndpoint();
    }
    _legacyTcpControlPort = port;
    emit legacyTcpControlPortChanged();
}

bool DigiviewConnection::connectToEndpoint()
{
    _mavlinkParserStatus = {};
    _mavlinkMessageBuffer = {};

    if (legacyTcpControlEnabled()) {
        _socket.close();
        if (!_legacyTcpActive) {
            _setConnected(false);
            _legacyTcpActive = true;
        }
        _setLastError(QString());
        const bool connected = _legacyTcpTransport->connectToEndpoint(_host, _legacyTcpControlPort);
        if (connected && _legacyTcpTransport->connected()) {
            _setConnected(true);
        }
        return connected;
    }

    QHostAddress remoteAddress;
    if (!_resolveRemoteAddress(remoteAddress)) {
        return false;
    }

    if (_legacyTcpActive) {
        _legacyTcpActive = false;
        _legacyTcpTransport->disconnectFromEndpoint();
        _setConnected(false);
    }

    _socket.close();

    if (!_socket.bind(QHostAddress::AnyIPv4, _listenPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        _setLastError(
            tr("Failed to bind Digiview UDP socket to port %1: %2").arg(_listenPort).arg(_socket.errorString()));
        _setConnected(false);
        return false;
    }

    _setLastError(QString());
    _setConnected(true);

    if (!_sendUdpRegistration(remoteAddress)) {
        _setConnected(false);
        _socket.close();
        return false;
    }

    qCDebug(DigiviewConnectionLog)
        << "Digiview UDP bound on" << _listenPort
        << "sending to" << remoteAddress.toString() << _port;

    return true;
}

void DigiviewConnection::disconnectFromEndpoint()
{
    _legacyTcpTransport->cancelRestartDigiView();
    _mavlinkParserStatus = {};
    _mavlinkMessageBuffer = {};
    _restartObservationGeneration.reset();
    _restartHeartbeatLossTimer.stop();
    _restartHeartbeatObserved = false;

    if (_socket.isOpen()) {
        _socket.close();
    }
    if (_legacyTcpActive) {
        _legacyTcpTransport->parkConnection();
    } else {
        _legacyTcpTransport->disconnectFromEndpoint();
    }

    _setConnected(false);
}

bool DigiviewConnection::sendMessage(const mavlink_message_t& message)
{
    if (!_connected) {
        return false;
    }

    if (_legacyTcpActive) {
        const bool sent = _legacyTcpTransport->sendMessage(message);
        if (sent) {
            _setLastError(QString());
        }
        return sent;
    }

    QHostAddress remoteAddress;
    if (!_resolveRemoteAddress(remoteAddress)) {
        return false;
    }

    std::array<uint8_t, MAVLINK_MAX_PACKET_LEN> buffer {};
    const uint16_t messageLength = mavlink_msg_to_send_buffer(buffer.data(), &message);

    const qint64 bytesWritten = _socket.writeDatagram(
        reinterpret_cast<const char*>(buffer.data()),
        messageLength,
        remoteAddress,
        _port);

    if (bytesWritten != messageLength) {
        _setLastError(tr("Failed to send Digiview datagram to %1:%2: %3")
                          .arg(remoteAddress.toString())
                          .arg(_port)
                          .arg(_socket.errorString()));
        return false;
    }

    _setLastError(QString());
    return true;
}

bool DigiviewConnection::restartDigiView(quint64 generation)
{
    return _legacyTcpTransport->restartDigiView(_host, _legacyTcpControlPort, generation);
}

void DigiviewConnection::cancelRestartDigiView()
{
    _legacyTcpTransport->cancelRestartDigiView();
}

void DigiviewConnection::_readPendingDatagrams()
{
    QHostAddress remoteAddress;
    const bool filterByHost = _resolveRemoteAddress(remoteAddress);

    while (_socket.hasPendingDatagrams()) {
        const qint64 datagramSize = _socket.pendingDatagramSize();
        if (datagramSize <= 0) {
            break;
        }

        QByteArray datagram(static_cast<int>(datagramSize), Qt::Uninitialized);
        QHostAddress senderAddress;
        quint16 senderPort = 0;
        const qint64 bytesRead = _socket.readDatagram(datagram.data(), datagram.size(), &senderAddress, &senderPort);
        if (bytesRead <= 0) {
            continue;
        }

        if (filterByHost && !senderAddress.isEqual(remoteAddress, QHostAddress::TolerantConversion)) {
            continue;
        }

        // MVP: only host filtering is enforced. Digiview replies may legitimately come
        // from a different source port than the configured send port, so filtering by
        // sender port here would be more fragile than helpful.
        Q_UNUSED(senderPort);

        mavlink_message_t message {};
        for (int i = 0; i < bytesRead; ++i) {
            if (mavlink_frame_char_buffer(
                    &_mavlinkMessageBuffer,
                     &_mavlinkParserStatus,
                    static_cast<uint8_t>(datagram.at(i)),
                    &message,
                     &_mavlinkParserStatus) == MAVLINK_FRAMING_OK) {
                if ((message.msgid == MAVLINK_MSG_ID_HEARTBEAT)
                    && (message.sysid == kDigiviewSystemId) && (message.compid == kDigiviewComponentId)) {
                    if (_restartObservationGeneration) {
                        _restartHeartbeatObserved = true;
                        _restartHeartbeatLossTimer.start(kRestartHeartbeatLossTimeoutMs);
                        emit restartReturnObserved(*_restartObservationGeneration);
                    }
                    _validHeartbeatSeen = true;
                }
                emit messageReceived(message);
            }
        }
    }
}

bool DigiviewConnection::_sendUdpRegistration(const QHostAddress& remoteAddress)
{
    mavlink_message_t heartbeat {};
    mavlink_msg_heartbeat_pack(
        MAVLinkProtocol::instance()->getSystemId(),
        MAVLinkProtocol::instance()->getComponentId(),
        &heartbeat,
        MAV_TYPE_GCS,
        MAV_AUTOPILOT_INVALID,
        MAV_MODE_MANUAL_ARMED,
        0,
        MAV_STATE_ACTIVE);

    std::array<uint8_t, MAVLINK_MAX_PACKET_LEN> buffer {};
    const uint16_t messageLength = mavlink_msg_to_send_buffer(buffer.data(), &heartbeat);
    const qint64 bytesWritten = _socket.writeDatagram(
        reinterpret_cast<const char*>(buffer.data()), messageLength, remoteAddress, kDigiviewRouterPort);
    if (bytesWritten != messageLength) {
        _setLastError(tr("Failed to register QGC with Digiview router %1:%2: %3")
                          .arg(remoteAddress.toString())
                          .arg(kDigiviewRouterPort)
                          .arg(_socket.errorString()));
        return false;
    }

    qCDebug(DigiviewConnectionLog) << "Sent QGC UDP registration heartbeat to"
                                   << remoteAddress.toString() << kDigiviewRouterPort;
    return true;
}

void DigiviewConnection::_socketErrorOccurred(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);

    _setLastError(_socket.errorString());
}

void DigiviewConnection::_emitLegacyTcpHeartbeat()
{
    if (!_legacyTcpActive || !_legacyTcpTransport->connected()) {
        return;
    }

    mavlink_message_t heartbeat {};
    mavlink_msg_heartbeat_pack(kDigiviewSystemId, kDigiviewComponentId, &heartbeat, MAV_TYPE_ONBOARD_CONTROLLER,
                               MAV_AUTOPILOT_INVALID, 0, 0, MAV_STATE_ACTIVE);
    if (_restartObservationGeneration) {
        emit restartReturnObserved(*_restartObservationGeneration);
    }
    emit messageReceived(heartbeat);
}

void DigiviewConnection::armRestartReturnObservation(quint64 generation)
{
    _restartObservationGeneration = generation;
    _restartHeartbeatObserved = false;
    _restartHeartbeatLossTimer.stop();
    if (!_legacyTcpActive && _connected && _validHeartbeatSeen) {
        _restartHeartbeatObserved = true;
        _restartHeartbeatLossTimer.start(kRestartHeartbeatLossTimeoutMs);
    }
}

void DigiviewConnection::disarmRestartReturnObservation()
{
    _restartObservationGeneration.reset();
    _restartHeartbeatLossTimer.stop();
    _restartHeartbeatObserved = false;
}

void DigiviewConnection::_restartHeartbeatLossTimeout()
{
    if (_restartObservationGeneration && !_legacyTcpActive && _restartHeartbeatObserved) {
        emit restartTransportDownObserved(*_restartObservationGeneration);
    }
}

void DigiviewConnection::_setConnected(bool connected)
{
    if (connected == _connected) {
        return;
    }

    _connected = connected;
    emit connectedChanged();
}

void DigiviewConnection::_setLastError(const QString& error)
{
    if (error == _lastError) {
        return;
    }

    _lastError = error;
    emit lastErrorChanged();

    if (!_lastError.isEmpty()) {
        emit errorOccurred(_lastError);
    }
}

bool DigiviewConnection::_resolveRemoteAddress(QHostAddress& remoteAddress)
{
    const QString trimmedHost = _host.trimmed();
    if (trimmedHost.isEmpty()) {
        _setLastError(tr("Digiview host is empty"));
        return false;
    }

    if (remoteAddress.setAddress(trimmedHost)) {
        return true;
    }

    const QHostInfo hostInfo = QHostInfo::fromName(trimmedHost);
    if (!hostInfo.addresses().isEmpty()) {
        remoteAddress = hostInfo.addresses().constFirst();
        return true;
    }

    _setLastError(tr("Failed to resolve Digiview host '%1'").arg(_host));
    return false;
}
