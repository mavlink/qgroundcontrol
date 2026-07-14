#include "DigiviewConnection.h"

#include "QGCLoggingCategory.h"

#include <QtCore/QByteArray>
#include <QtNetwork/QHostInfo>

#include <array>

namespace {

constexpr uint8_t kDigiviewCrc8Polynomial = 0xA7;
constexpr uint8_t kDigiviewMavlink1Magic = 0xFE;
constexpr uint8_t kDigiviewMavlink2Magic = 0xFD;
constexpr uint16_t kDigiviewMavlink2IncompatFlagsOffset = 2;
constexpr uint16_t kDigiviewMavlink1HeaderLength = 6;
constexpr uint16_t kDigiviewMavlink2HeaderLength = 10;
constexpr uint16_t kDigiviewChecksumBytes = 2;

// Matches the DigiView BLUETOOTH CRC8 preset in message-definitions/msg_defs.hpp,
// but stays local to avoid pulling the full native protocol header into this TU.
uint8_t _reflectByte(uint8_t value)
{
    uint8_t reflection = 0;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        if ((value & (1U << bit)) != 0U) {
            reflection |= static_cast<uint8_t>(1U << (7U - bit));
        }
    }

    return reflection;
}

uint8_t _digiviewCrc8(const uint8_t* data, uint16_t length)
{
    uint8_t checksum = 0;

    for (uint16_t index = 0; index < length; ++index) {
        checksum ^= _reflectByte(data[index]);

        for (uint8_t bit = 0; bit < 8; ++bit) {
            if ((checksum & 0x80U) != 0U) {
                checksum = static_cast<uint8_t>((checksum << 1U) ^ kDigiviewCrc8Polynomial);
            } else {
                checksum = static_cast<uint8_t>(checksum << 1U);
            }
        }
    }

    return _reflectByte(checksum);
}

} // namespace

QGC_LOGGING_CATEGORY(DigiviewConnectionLog, "Digiview.Connection")

DigiviewConnection::DigiviewConnection(QObject* parent)
    : QObject(parent)
{
    connect(&_socket, &QUdpSocket::readyRead, this, &DigiviewConnection::_readPendingDatagrams);
    connect(&_socket, &QUdpSocket::errorOccurred, this, &DigiviewConnection::_socketErrorOccurred);
}

void DigiviewConnection::setHost(const QString& host)
{
    const QString trimmedHost = host.trimmed();
    if (trimmedHost == _host) {
        return;
    }

    _host = trimmedHost;
    emit hostChanged();

    if (_connected) {
        (void) connectToEndpoint();
    }
}

void DigiviewConnection::setPort(quint16 port)
{
    if (port == _port) {
        return;
    }

    _port = port;
    emit portChanged();
}

void DigiviewConnection::setListenPort(quint16 listenPort)
{
    if (listenPort == _listenPort) {
        return;
    }

    _listenPort = listenPort;
    emit listenPortChanged();

    if (_connected) {
        (void) connectToEndpoint();
    }
}

bool DigiviewConnection::connectToEndpoint()
{
    QHostAddress remoteAddress;
    if (!_resolveRemoteAddress(remoteAddress)) {
        return false;
    }

    _socket.close();

    if (!_socket.bind(QHostAddress::AnyIPv4, _listenPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        _setLastError(tr("Failed to bind Digiview UDP socket to port %1: %2").arg(_listenPort).arg(_socket.errorString()));
        _setConnected(false);
        return false;
    }

    _setLastError(QString());
    _setConnected(true);

    qCDebug(DigiviewConnectionLog)
        << "Digiview UDP bound on" << _listenPort
        << "sending to" << remoteAddress.toString() << _port;

    return true;
}

void DigiviewConnection::disconnectFromEndpoint()
{
    if (_socket.isOpen()) {
        _socket.close();
    }

    _setConnected(false);
}

bool DigiviewConnection::sendMessage(const mavlink_message_t& message)
{
    if (!_connected && !connectToEndpoint()) {
        return false;
    }

    QHostAddress remoteAddress;
    if (!_resolveRemoteAddress(remoteAddress)) {
        return false;
    }

    std::array<uint8_t, MAVLINK_MAX_PACKET_LEN> buffer {};
    const uint16_t messageLength = mavlink_msg_to_send_buffer(buffer.data(), &message);

    uint16_t checksumOffset = 0;
    switch (buffer[0]) {
    case kDigiviewMavlink1Magic:
        checksumOffset = static_cast<uint16_t>(kDigiviewMavlink1HeaderLength + buffer[1]);
        break;
    case kDigiviewMavlink2Magic:
        if ((buffer[kDigiviewMavlink2IncompatFlagsOffset] & MAVLINK_IFLAG_SIGNED) != 0U) {
            _setLastError(tr("Failed to send Digiview datagram: signed MAVLink 2 packets are not supported on this transport"));
            return false;
        }

        checksumOffset = static_cast<uint16_t>(kDigiviewMavlink2HeaderLength + buffer[1]);
        break;
    default:
        _setLastError(tr("Failed to send Digiview datagram: unsupported MAVLink framing byte %1").arg(buffer[0]));
        return false;
    }

    if (messageLength < (checksumOffset + kDigiviewChecksumBytes)) {
        _setLastError(tr("Failed to send Digiview datagram: serialized MAVLink packet is shorter than its checksum trailer"));
        return false;
    }

    const uint8_t originalChecksumLow = buffer[checksumOffset];
    const uint8_t originalChecksumHigh = buffer[checksumOffset + 1U];
    const uint8_t checksum = _digiviewCrc8(buffer.data(), checksumOffset);
    buffer[checksumOffset] = checksum;

    const uint16_t datagramLength = static_cast<uint16_t>(messageLength - 1U);

    const qint64 bytesWritten = _socket.writeDatagram(
        reinterpret_cast<const char*>(buffer.data()),
        datagramLength,
        remoteAddress,
        _port);
    const qint64 transmittedLength = (bytesWritten > 0) ? bytesWritten : 0;
    const QString transmittedBytesHex = (transmittedLength > 0)
        ? QString::fromLatin1(
              QByteArray::fromRawData(
                  reinterpret_cast<const char*>(buffer.data()),
                  static_cast<qsizetype>(transmittedLength))
                  .toHex(' '))
        : QStringLiteral("<none>");

    qCWarning(DigiviewConnectionLog).noquote()
        << QStringLiteral("Digiview TX msgid=%1 frame=0x%2 payloadLen=%3 checksumOffset=%4 mavCrc=[0x%5 0x%6] digiCrc8=0x%7 sentLen=%8")
               .arg(message.msgid)
               .arg(buffer[0], 2, 16, QChar('0'))
               .arg(buffer[1])
               .arg(checksumOffset)
               .arg(originalChecksumLow, 2, 16, QChar('0'))
               .arg(originalChecksumHigh, 2, 16, QChar('0'))
               .arg(checksum, 2, 16, QChar('0'))
               .arg(transmittedLength);
    qCWarning(DigiviewConnectionLog).noquote()
        << QStringLiteral("Digiview TX bytes=%1").arg(transmittedBytesHex);

    if (bytesWritten != datagramLength) {
        _setLastError(tr("Failed to send Digiview datagram to %1:%2: %3")
                          .arg(remoteAddress.toString())
                          .arg(_port)
                          .arg(_socket.errorString()));
        return false;
    }

    _setLastError(QString());
    return true;
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
            qCDebug(DigiviewConnectionLog)
                << "Ignoring Digiview datagram from unexpected host"
                << senderAddress.toString()
                << "expected" << remoteAddress.toString();
            continue;
        }

        // MVP: only host filtering is enforced. Digiview replies may legitimately come
        // from a different source port than the configured send port, so filtering by
        // sender port here would be more fragile than helpful.
        Q_UNUSED(senderPort);

        mavlink_message_t message;
        for (int i = 0; i < bytesRead; ++i) {
            if (mavlink_parse_char(MAVLINK_COMM_0, static_cast<uint8_t>(datagram.at(i)), &message, &_mavlinkStatus)) {
                emit messageReceived(message);
            }
        }
    }
}

void DigiviewConnection::_socketErrorOccurred(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);

    _setLastError(_socket.errorString());
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
