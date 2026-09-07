#include "DigiviewLegacyTcpTransport.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(DigiviewLegacyTcpTransportLog, "Digiview.LegacyTcp.Transport")

namespace {
constexpr int kRestartConnectTimeoutMs = 1000;
constexpr int kRestartRetryIntervalMs = 250;
constexpr int kRestartMaxAttempts = 8;
}

DigiviewLegacyTcpTransport::DigiviewLegacyTcpTransport(QObject* parent)
    : QObject(parent)
{
    connect(&_socket, &QTcpSocket::connected, this, [this] {
        _disconnectRequested = false;
        _parked = false;
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
    connect(&_restartSocket, &QTcpSocket::connected, this, &DigiviewLegacyTcpTransport::_restartSocketConnected);
    connect(&_restartSocket, &QTcpSocket::disconnected, this, &DigiviewLegacyTcpTransport::_restartSocketDisconnected);
    connect(&_restartSocket, &QTcpSocket::errorOccurred, this,
            &DigiviewLegacyTcpTransport::_restartSocketErrorOccurred);
    connect(&_restartSocket, &QTcpSocket::bytesWritten, this,
            &DigiviewLegacyTcpTransport::_restartSocketBytesWritten);
    connect(&_restartTimer, &QTimer::timeout, this, &DigiviewLegacyTcpTransport::_restartRetry);
    _restartTimer.setSingleShot(true);
}

bool DigiviewLegacyTcpTransport::connectToEndpoint(const QString& host, quint16 port)
{
    if (connected() && _parked) {
        _parked = false;
        _disconnectRequested = false;
        emit connectedToEndpoint();
        return true;
    }
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
    _parked = false;
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
    _parked = false;
}

void DigiviewLegacyTcpTransport::parkConnection()
{
    if (!connected()) {
        disconnectFromEndpoint();
        return;
    }

    _receiveBuffer.clear();
    _disconnectRequested = true;
    _parked = true;
    qCDebug(DigiviewLegacyTcpTransportLog) << "Parked DigiView legacy TCP control connection";
}

bool DigiviewLegacyTcpTransport::restartDigiView(const QString& host, quint16 port, quint64 generation)
{
    if (_restartInProgress) {
        return false;
    }
    _restartHost = host.trimmed();
    _restartPort = port;
    _restartAttempts = 0;
    _restartQuitSent = false;
    _restartGeneration = generation;
    _restartLastError.clear();
    _restartRecord.clear();
    _restartWriteOffset = 0;
    if (_restartHost.isEmpty() || (_restartPort == 0)) {
        emit restartFailed(_restartGeneration, tr("Invalid DigiView restart endpoint"));
        return false;
    }
    _restartInProgress = true;
    _restartRetry();
    return true;
}

void DigiviewLegacyTcpTransport::cancelRestartDigiView()
{
    _restartTimer.stop();
    _restartInProgress = false;
    _restartQuitSent = false;
    _restartConnectingAttempt = false;
    _restartRecord.clear();
    _restartWriteOffset = 0;
    _restartSocket.abort();
}

void DigiviewLegacyTcpTransport::_restartRetry()
{
    if (!_restartInProgress || _restartQuitSent) return;
    if (_restartConnectingAttempt) {
        _restartConnectingAttempt = false;
        _restartSocket.abort();
        _restartTimer.start(kRestartRetryIntervalMs);
        return;
    }
    if (_restartAttempts++ >= kRestartMaxAttempts) {
        _restartInProgress = false;
        _restartTimer.stop();
        _restartConnectingAttempt = false;
        _restartRecord.clear();
        _restartWriteOffset = 0;
        _restartSocket.abort();
        const QString error = _restartLastError.isEmpty()
            ? tr("DigiView restart TCP side channel could not reach %1:%2").arg(_restartHost).arg(_restartPort)
            : tr("DigiView restart TCP side channel failed: %1").arg(_restartLastError);
        emit restartFailed(_restartGeneration, error);
        return;
    }
    _restartSocket.abort();
    _restartConnectingAttempt = true;
    _restartSocket.connectToHost(_restartHost, _restartPort);
    _restartTimer.start(kRestartConnectTimeoutMs);
}

void DigiviewLegacyTcpTransport::_restartSocketConnected()
{
    if (!_restartInProgress || _restartQuitSent) return;
    _restartConnectingAttempt = false;
    _restartTimer.stop();
    const QByteArray record = _adapter.encodeRestartQuit();
    _restartRecord = record;
    _restartWriteOffset = 0;
    _restartSocketBytesWritten(0);
}

void DigiviewLegacyTcpTransport::_restartSocketBytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes);
    if (!_restartInProgress || _restartQuitSent || _restartRecord.isEmpty()) return;

    while (_restartWriteOffset < _restartRecord.size()) {
        const qsizetype remaining = _restartRecord.size() - _restartWriteOffset;
        const qint64 written = _restartSocket.write(
            _restartRecord.constData() + _restartWriteOffset, remaining);
        if (written <= 0) {
            _restartLastError = tr("Failed to write DigiView restart request: %1").arg(_restartSocket.errorString());
            _restartInProgress = false;
            _restartRecord.clear();
            emit restartFailed(_restartGeneration, _restartLastError);
            _restartSocket.abort();
            return;
        }
        _restartWriteOffset += written;
        if (written < remaining) break;
    }

    if ((_restartWriteOffset == _restartRecord.size()) && (_restartSocket.bytesToWrite() == 0)) {
        _restartQuitSent = true;
        _restartRecord.clear();
        emit restartQuitSent(_restartGeneration);
        _restartSocket.disconnectFromHost();
    }
}

void DigiviewLegacyTcpTransport::_restartSocketDisconnected()
{
    if (_restartInProgress && !_restartQuitSent) {
        _restartConnectingAttempt = false;
        _restartTimer.start(kRestartRetryIntervalMs);
    } else if (_restartQuitSent) {
        _restartInProgress = false;
    }
}

void DigiviewLegacyTcpTransport::_restartSocketErrorOccurred(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    _restartLastError = _restartSocket.errorString();
    if (_restartInProgress && !_restartQuitSent && !_restartTimer.isActive()) _restartTimer.start(kRestartRetryIntervalMs);
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
    if (!connected() || _parked) {
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
            if (!error.isEmpty()) {
                qCWarning(DigiviewLegacyTcpTransportLog) << error;
                emit errorOccurred(error);
            }
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
