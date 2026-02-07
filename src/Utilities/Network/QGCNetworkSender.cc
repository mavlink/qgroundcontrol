#include "QGCNetworkSender.h"
#include <QtCore/QLoggingCategory>

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QFile>
#include <QtCore/QMetaObject>
#include <QtCore/QtEndian>
#include <QtNetwork/QSslError>

Q_STATIC_LOGGING_CATEGORY(QGCNetworkSenderLog, "Utilities.Network.QGCNetworkSender", QtWarningMsg)

QGCNetworkSender::QGCNetworkSender(QObject* parent)
    : QObject(parent)
{
}

QGCNetworkSender::~QGCNetworkSender()
{
    stop();
}

void QGCNetworkSender::setEndpoint(const QHostAddress& host, quint16 port)
{
    bool shouldWake = false;
    {
        QMutexLocker locker(&_mutex);
        if (_host == host && _port == port) {
            return;
        }
        _host = host;
        _port = port;
        if (_connected) {
            _endpointChanged = true;
            shouldWake = true;
        }
        qCDebug(QGCNetworkSenderLog) << "Endpoint set to" << host.toString() << ":" << port;
    }

    if (shouldWake) {
        _cv.wakeOne();
    }
}

QHostAddress QGCNetworkSender::host() const
{
    QMutexLocker locker(&_mutex);
    return _host;
}

quint16 QGCNetworkSender::port() const
{
    QMutexLocker locker(&_mutex);
    return _port;
}

void QGCNetworkSender::setProtocol(Protocol protocol)
{
    bool shouldWake = false;
    {
        QMutexLocker locker(&_mutex);
        if (_protocol != protocol) {
            _protocol = protocol;
            _protocolChanged = true;
            shouldWake = true;
            qCDebug(QGCNetworkSenderLog) << "Protocol set to" << static_cast<int>(protocol);
        }
    }

    if (shouldWake) {
        _cv.wakeOne();
    }
}

QGCNetworkSender::Protocol QGCNetworkSender::protocol() const
{
    QMutexLocker locker(&_mutex);
    return _protocol;
}

void QGCNetworkSender::start()
{
    {
        QMutexLocker locker(&_mutex);
        if (_running) {
            return;
        }
        _running = true;
    }

    _future = QtConcurrent::run(&QGCNetworkSender::_senderLoop, this);
    qCDebug(QGCNetworkSenderLog) << "Sender started via QtConcurrent";
}

void QGCNetworkSender::stop()
{
    {
        QMutexLocker locker(&_mutex);
        if (!_running) {
            return;
        }
        _running = false;
    }

    _cv.wakeOne();

    if (_future.isValid()) {
        _future.waitForFinished();
    }

    qCDebug(QGCNetworkSenderLog) << "Sender stopped";
}

bool QGCNetworkSender::isRunning() const
{
    QMutexLocker locker(&_mutex);
    return _running;
}

void QGCNetworkSender::send(const QByteArray& data)
{
    if (data.isEmpty()) {
        return;
    }

    {
        QMutexLocker locker(&_mutex);
        if (!_running) {
            return;
        }

        _queue.enqueue(data);
        _pendingBytes += data.size();

        // Drop oldest if over limit
        while (_pendingBytes > _maxPendingBytes && !_queue.isEmpty()) {
            _pendingBytes -= _queue.dequeue().size();
        }
    }

    _cv.wakeOne();
}

void QGCNetworkSender::flush()
{
    {
        QMutexLocker locker(&_mutex);
        if (!_running) {
            return;
        }
        _flushRequested = true;
    }

    _cv.wakeOne();

    QMutexLocker locker(&_mutex);
    while (_flushRequested && _running) {
        if (!_cv.wait(&_mutex, 500)) {
            break; // Timeout
        }
    }
}

bool QGCNetworkSender::isConnected() const
{
    QMutexLocker locker(&_mutex);
    return _connected;
}

bool QGCNetworkSender::hasError() const
{
    QMutexLocker locker(&_mutex);
    return _hasError;
}

void QGCNetworkSender::clearError()
{
    {
        QMutexLocker locker(&_mutex);
        if (!_hasError) {
            return;
        }
        _hasError = false;
    }

    QMetaObject::invokeMethod(this, [this]() {
        emit errorCleared();
    }, Qt::QueuedConnection);
}

bool QGCNetworkSender::lastSendSucceeded() const
{
    QMutexLocker locker(&_mutex);
    return _lastSendSucceeded;
}

void QGCNetworkSender::setMaxPendingBytes(qint64 bytes)
{
    QMutexLocker locker(&_mutex);
    _maxPendingBytes = qMax(static_cast<qint64>(4096), bytes);
}

qint64 QGCNetworkSender::maxPendingBytes() const
{
    QMutexLocker locker(&_mutex);
    return _maxPendingBytes;
}

void QGCNetworkSender::setConnectTimeout(int ms)
{
    QMutexLocker locker(&_mutex);
    _connectTimeout = qMax(100, ms);
}

int QGCNetworkSender::connectTimeout() const
{
    QMutexLocker locker(&_mutex);
    return _connectTimeout;
}

void QGCNetworkSender::setReconnectInterval(int ms)
{
    QMutexLocker locker(&_mutex);
    _reconnectInterval = qMax(100, ms);
}

int QGCNetworkSender::reconnectInterval() const
{
    QMutexLocker locker(&_mutex);
    return _reconnectInterval;
}

void QGCNetworkSender::setTlsVerifyPeer(bool verify)
{
    QMutexLocker locker(&_mutex);
    if (_tlsVerifyPeer == verify) {
        return;
    }

    _tlsVerifyPeer = verify;
    _sslConfig.setPeerVerifyMode(verify ? QSslSocket::VerifyPeer : QSslSocket::VerifyNone);
    qCDebug(QGCNetworkSenderLog) << "TLS peer verification" << (verify ? "enabled" : "disabled");
}

bool QGCNetworkSender::tlsVerifyPeer() const
{
    QMutexLocker locker(&_mutex);
    return _tlsVerifyPeer;
}

void QGCNetworkSender::setTlsCaCertificates(const QList<QSslCertificate>& certs)
{
    QMutexLocker locker(&_mutex);
    _sslConfig.setCaCertificates(certs);
    qCDebug(QGCNetworkSenderLog) << "Set" << certs.size() << "CA certificates";
}

void QGCNetworkSender::setTlsClientCertificate(const QSslCertificate& cert, const QSslKey& key)
{
    QMutexLocker locker(&_mutex);
    _sslConfig.setLocalCertificate(cert);
    _sslConfig.setPrivateKey(key);
    qCDebug(QGCNetworkSenderLog) << "Set client certificate for mutual TLS";
}

bool QGCNetworkSender::loadTlsCaCertificates(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(QGCNetworkSenderLog) << "Failed to open CA certificate file:" << filePath;
        return false;
    }

    QList<QSslCertificate> certs = QSslCertificate::fromDevice(&file, QSsl::Pem);
    if (certs.isEmpty()) {
        qCWarning(QGCNetworkSenderLog) << "No valid certificates found in:" << filePath;
        return false;
    }

    setTlsCaCertificates(certs);
    return true;
}

bool QGCNetworkSender::loadTlsClientCertificate(const QString& certPath, const QString& keyPath)
{
    QFile certFile(certPath);
    if (!certFile.open(QIODevice::ReadOnly)) {
        qCWarning(QGCNetworkSenderLog) << "Failed to open client certificate file:" << certPath;
        return false;
    }

    QList<QSslCertificate> certs = QSslCertificate::fromDevice(&certFile, QSsl::Pem);
    if (certs.isEmpty()) {
        qCWarning(QGCNetworkSenderLog) << "No valid certificate found in:" << certPath;
        return false;
    }

    QFile keyFile(keyPath);
    if (!keyFile.open(QIODevice::ReadOnly)) {
        qCWarning(QGCNetworkSenderLog) << "Failed to open private key file:" << keyPath;
        return false;
    }

    QSslKey key(&keyFile, QSsl::Rsa, QSsl::Pem);
    if (key.isNull()) {
        keyFile.seek(0);
        key = QSslKey(&keyFile, QSsl::Ec, QSsl::Pem);
    }

    if (key.isNull()) {
        qCWarning(QGCNetworkSenderLog) << "No valid private key found in:" << keyPath;
        return false;
    }

    setTlsClientCertificate(certs.first(), key);
    return true;
}

void QGCNetworkSender::_senderLoop()
{
    _udpSocket = new QUdpSocket();
    _tcpSocket = new QTcpSocket();
    _sslSocket = new QSslSocket();

    QQueue<QByteArray> batch;

    while (true) {
        // Wait for data
        {
            QMutexLocker locker(&_mutex);

            while (_queue.isEmpty() && !_flushRequested && !_endpointChanged && !_protocolChanged && _running) {
                _cv.wait(&_mutex, kSendTimeoutMs);
            }

            if (!_running && _queue.isEmpty()) {
                break;
            }

            batch.swap(_queue);
            _pendingBytes = 0;
        }

        // Get endpoint and protocol
        QHostAddress host;
        quint16 port;
        Protocol proto;
        bool endpointChanged = false;
        bool protocolChanged = false;
        {
            QMutexLocker locker(&_mutex);
            host = _host;
            port = _port;
            proto = _protocol;
            endpointChanged = _endpointChanged;
            protocolChanged = _protocolChanged;
            _endpointChanged = false;
            _protocolChanged = false;
        }

        // Disconnect stale TCP/TLS if endpoint changed
        if (endpointChanged || protocolChanged) {
            _disconnect();
        }

        if (host.isNull() || port == 0) {
            batch.clear();
            continue;
        }

        // Send batch
        while (!batch.isEmpty()) {
            const QByteArray data = batch.dequeue();
            bool success = false;

            switch (proto) {
            case Protocol::UDP:
                success = _sendUdp(data);
                break;
            case Protocol::TCP: {
                bool connected;
                {
                    QMutexLocker locker(&_mutex);
                    connected = _connected;
                }
                if (!connected) {
                    _connectTcp();
                }
                {
                    QMutexLocker locker(&_mutex);
                    connected = _connected;
                }
                if (connected) {
                    success = _sendTcp(data, proto);
                }
                break;
            }
            case Protocol::TLS: {
                bool connected;
                {
                    QMutexLocker locker(&_mutex);
                    connected = _connected;
                }
                if (!connected) {
                    _connectTls();
                }
                {
                    QMutexLocker locker(&_mutex);
                    connected = _connected;
                }
                if (connected) {
                    success = _sendTcp(data, proto);
                }
                break;
            }
            }

            {
                QMutexLocker locker(&_mutex);
                _lastSendSucceeded = success;
            }

            if (success) {
                QMetaObject::invokeMethod(this, [this, size = data.size()]() {
                    emit dataSent(size);
                }, Qt::QueuedConnection);
            }
        }

        // Handle flush
        {
            QMutexLocker locker(&_mutex);
            if (_flushRequested) {
                _flushRequested = false;
                _cv.wakeAll();
            }
        }
    }

    // Cleanup
    _disconnect();
    delete _sslSocket;
    _sslSocket = nullptr;
    delete _tcpSocket;
    _tcpSocket = nullptr;
    delete _udpSocket;
    _udpSocket = nullptr;
}

bool QGCNetworkSender::_sendUdp(const QByteArray& data)
{
    QHostAddress host;
    quint16 port;
    {
        QMutexLocker locker(&_mutex);
        host = _host;
        port = _port;
    }

    const qint64 sent = _udpSocket->writeDatagram(data, host, port);
    if (sent < 0) {
        _emitError(QStringLiteral("UDP send failed: %1").arg(_udpSocket->errorString()));
        return false;
    }
    return true;
}

bool QGCNetworkSender::_sendTcp(const QByteArray& data, Protocol protocol)
{
    bool connected;
    {
        QMutexLocker locker(&_mutex);
        connected = _connected;
    }

    QAbstractSocket* socket = (protocol == Protocol::TLS)
        ? static_cast<QAbstractSocket*>(_sslSocket)
        : static_cast<QAbstractSocket*>(_tcpSocket);

    if (!socket || !connected) {
        return false;
    }

    // Length-prefix framing: 4-byte big-endian length followed by payload.
    // This is safe for binary (compressed) data unlike newline delimiters.
    QByteArray packet;
    packet.reserve(4 + data.size());
    const quint32 len = qToBigEndian(static_cast<quint32>(data.size()));
    packet.append(reinterpret_cast<const char*>(&len), sizeof(len));
    packet.append(data);

    qint64 totalWritten = 0;
    while (totalWritten < packet.size()) {
        const qint64 written = socket->write(packet.constData() + totalWritten, packet.size() - totalWritten);
        if (written < 0) {
            qCDebug(QGCNetworkSenderLog) << "TCP write failed:" << socket->errorString();
            _disconnect();
            return false;
        }
        totalWritten += written;
    }

    if (!socket->flush()) {
        if (socket->state() != QAbstractSocket::ConnectedState) {
            _disconnect();
            return false;
        }
    }

    return true;
}

bool QGCNetworkSender::_connectTcp()
{
    {
        QMutexLocker locker(&_mutex);
        if (_connected) {
            return true;
        }

        // Rate limit connection attempts
        if (_lastConnectAttempt.isValid() && _lastConnectAttempt.elapsed() < _reconnectInterval) {
            return false;
        }
    }

    QHostAddress host;
    quint16 port;
    int timeout;
    {
        QMutexLocker locker(&_mutex);
        host = _host;
        port = _port;
        timeout = _connectTimeout;
        _lastConnectAttempt.start();
    }

    qCDebug(QGCNetworkSenderLog) << "Connecting TCP to" << host.toString() << ":" << port;

    if (_tcpSocket->state() != QAbstractSocket::UnconnectedState) {
        _tcpSocket->abort();
    }
    _tcpSocket->connectToHost(host, port);
    const bool success = _tcpSocket->waitForConnected(timeout);

    if (success) {
        {
            QMutexLocker locker(&_mutex);
            _connected = true;
        }
        qCDebug(QGCNetworkSenderLog) << "TCP connected";
        QMetaObject::invokeMethod(this, [this]() {
            emit connected();
        }, Qt::QueuedConnection);
    } else {
        _emitError(QStringLiteral("TCP connection failed: %1").arg(_tcpSocket->errorString()));
    }

    return success;
}

bool QGCNetworkSender::_connectTls()
{
    {
        QMutexLocker locker(&_mutex);
        if (_connected) {
            return true;
        }

        // Rate limit connection attempts
        if (_lastConnectAttempt.isValid() && _lastConnectAttempt.elapsed() < _reconnectInterval) {
            return false;
        }
    }

    QHostAddress host;
    quint16 port;
    int timeout;
    QSslConfiguration sslConfig;
    {
        QMutexLocker locker(&_mutex);
        host = _host;
        port = _port;
        timeout = _connectTimeout;
        sslConfig = _sslConfig;
        _lastConnectAttempt.start();
    }

    qCDebug(QGCNetworkSenderLog) << "Connecting TLS to" << host.toString() << ":" << port;

    if (_sslSocket->state() != QAbstractSocket::UnconnectedState) {
        _sslSocket->abort();
    }
    _sslSocket->setSslConfiguration(sslConfig);
    _sslSocket->connectToHostEncrypted(host.toString(), port);
    const bool success = _sslSocket->waitForEncrypted(timeout);

    if (success) {
        {
            QMutexLocker locker(&_mutex);
            _connected = true;
        }
        qCDebug(QGCNetworkSenderLog) << "TLS connected";
        QMetaObject::invokeMethod(this, [this]() {
            emit connected();
        }, Qt::QueuedConnection);
    } else {
        _emitError(QStringLiteral("TLS connection failed: %1").arg(_sslSocket->errorString()));
        const auto errors = _sslSocket->sslHandshakeErrors();
        if (!errors.isEmpty()) {
            _handleSslErrors(errors);
        }
    }

    return success;
}

void QGCNetworkSender::_disconnect()
{
    if (_sslSocket && _sslSocket->state() != QAbstractSocket::UnconnectedState) {
        _sslSocket->disconnectFromHost();
        if (_sslSocket->state() != QAbstractSocket::UnconnectedState) {
            _sslSocket->waitForDisconnected(1000);
        }
    }

    if (_tcpSocket && _tcpSocket->state() != QAbstractSocket::UnconnectedState) {
        _tcpSocket->disconnectFromHost();
        if (_tcpSocket->state() != QAbstractSocket::UnconnectedState) {
            _tcpSocket->waitForDisconnected(1000);
        }
    }

    bool wasConnected;
    {
        QMutexLocker locker(&_mutex);
        wasConnected = _connected;
        _connected = false;
    }

    if (wasConnected) {
        qCDebug(QGCNetworkSenderLog) << "Disconnected";
        QMetaObject::invokeMethod(this, [this]() {
            emit disconnected();
        }, Qt::QueuedConnection);
    }
}

void QGCNetworkSender::_handleSslErrors(const QList<QSslError>& errors)
{
    QStringList errorStrings;
    for (const QSslError& error : errors) {
        errorStrings << error.errorString();
    }

    const QString message = errorStrings.join("; ");
    qCWarning(QGCNetworkSenderLog) << "SSL errors:" << message;

    QMetaObject::invokeMethod(this, [this, message]() {
        emit tlsErrorOccurred(message);
    }, Qt::QueuedConnection);
}

void QGCNetworkSender::_emitError(const QString& message)
{
    {
        QMutexLocker locker(&_mutex);
        _hasError = true;
    }
    qCWarning(QGCNetworkSenderLog) << message;

    QMetaObject::invokeMethod(this, [this, message]() {
        emit errorOccurred(message);
    }, Qt::QueuedConnection);
}
