#include "DigiviewLegacyTcpTransport.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(DigiviewLegacyTcpTransportLog, "Digiview.LegacyTcp.Transport")

DigiviewLegacyTcpTransport::DigiviewLegacyTcpTransport(QObject* parent)
    : QObject(parent)
{
    connect(&_socket, &QTcpSocket::connected, this, [this] {
        _disconnectRequested = false;
        emit connectedToEndpoint();
    });
    connect(&_socket, &QTcpSocket::disconnected, this, [this] {
        if (!_disconnectRequested) {
            emit errorOccurred(tr("DigiView legacy TCP control connection disconnected"));
        }
        emit disconnectedFromEndpoint();
    });
    connect(&_socket, &QTcpSocket::readyRead, this, &DigiviewLegacyTcpTransport::_readAvailableRecords);
    connect(&_socket, &QTcpSocket::errorOccurred, this, &DigiviewLegacyTcpTransport::_socketErrorOccurred);
}

bool DigiviewLegacyTcpTransport::connectToEndpoint(const QString& host, quint16 port)
{
    if (connected() || connecting()) {
        return true;
    }

    const QString endpointHost = host.trimmed();
    if (endpointHost.isEmpty() || (port == 0)) {
        emit errorOccurred(tr("Invalid DigiView legacy TCP control endpoint %1:%2").arg(endpointHost).arg(port));
        return false;
    }

    _receiveBuffer.clear();
    _disconnectRequested = false;
    _socket.connectToHost(endpointHost, port);
    qCDebug(DigiviewLegacyTcpTransportLog)
        << "Connecting to legacy DigiView TCP control endpoint" << endpointHost << port;
    return true;
}

void DigiviewLegacyTcpTransport::disconnectFromEndpoint()
{
    _receiveBuffer.clear();
    _disconnectRequested = true;
    // Closing the socket is sufficient. A legacy QUIT record would stop DigiView itself.
    _socket.abort();
}

bool DigiviewLegacyTcpTransport::connected() const
{
    return _socket.state() == QAbstractSocket::ConnectedState;
}

bool DigiviewLegacyTcpTransport::connecting() const
{
    return (_socket.state() == QAbstractSocket::HostLookupState)
        || (_socket.state() == QAbstractSocket::ConnectingState);
}

bool DigiviewLegacyTcpTransport::sendMessage(const mavlink_message_t& message)
{
    if (!connected()) {
        emit errorOccurred(tr("DigiView legacy TCP control endpoint is not connected"));
        return false;
    }

    QString error;
    const QByteArray record = _adapter.encode(message, error);
    if (record.isEmpty()) {
        qCWarning(DigiviewLegacyTcpTransportLog) << error;
        emit errorOccurred(error);
        return false;
    }

    qsizetype offset = 0;
    while (offset < record.size()) {
        const qint64 written = _socket.write(record.constData() + offset, record.size() - offset);
        if (written <= 0) {
            const QString socketError = tr("Failed to write DigiView legacy TCP record: %1").arg(_socket.errorString());
            emit errorOccurred(socketError);
            return false;
        }
        offset += written;
    }

    return true;
}

void DigiviewLegacyTcpTransport::_readAvailableRecords()
{
    _receiveBuffer.append(_socket.readAll());

    const qsizetype recordSize = DigiviewLegacyTcpAdapter::recordSize();
    qsizetype consumed = 0;
    while ((_receiveBuffer.size() - consumed) >= recordSize) {
        const QByteArrayView record(_receiveBuffer.constData() + consumed, recordSize);
        consumed += recordSize;

        mavlink_message_t mavlinkMessage {};
        QString error;
        const auto result = _adapter.decode(record, mavlinkMessage, error);
        if (result == DigiviewLegacyTcpAdapter::DecodeResult::Message) {
            emit messageReceived(mavlinkMessage);
        } else if (result == DigiviewLegacyTcpAdapter::DecodeResult::Error) {
            qCWarning(DigiviewLegacyTcpTransportLog) << error;
            emit errorOccurred(error);
        } else if (!error.isEmpty()) {
            qCDebug(DigiviewLegacyTcpTransportLog) << error;
        }
    }

    if (consumed > 0) {
        _receiveBuffer.remove(0, consumed);
    }
}

void DigiviewLegacyTcpTransport::_socketErrorOccurred(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    emit errorOccurred(_socket.errorString());
}
