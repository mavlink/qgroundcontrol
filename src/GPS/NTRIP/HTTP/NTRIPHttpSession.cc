#include "NTRIPHttpSession.h"

#include <utility>

#include <QtCore/QPointer>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QTcpSocket>

#include "NTRIPConfiguration.h"
#include "NTRIPTlsPolicy_p.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkClient.h"
#include "RuntimeScheduler.h"

QGC_LOGGING_CATEGORY(NTRIPHttpSessionLog, "GPS.NTRIP.NTRIPHttpSession")

NTRIPHttpSession::NTRIPHttpSession(QObject* parent, RuntimeScheduler* scheduler, NTRIPHttpPurpose purpose)
    : QObject(parent)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
    , _purpose(purpose)
    , _errorBodyTask(_scheduler, this)
    , _decoder(purpose)
{}

QString NTRIPHttpSession::open(const NTRIPConnectionConfig& config)
{
    if (_socket || _ended) {
        return {};
    }
    NTRIPHttpRequest request = NTRIPHttpRequest::build(config, _purpose);
    if (!request.error.isEmpty()) {
        return request.error;
    }
    _request = std::move(request.bytes);
    const auto port = static_cast<quint16>(config.port);
    _endpoint = QStringLiteral("%1:%2").arg(config.host).arg(port);
    _pinnedCertificate = config.pinnedCertificate;
    _allowSelfSignedCerts = config.allowSelfSignedCerts;
    qCDebug(NTRIPHttpSessionLog) << "connectToHost" << config.host << ":" << config.port << "TLS" << config.useTls;
    if (!config.useTls) {
        auto* socket = new QTcpSocket(this);
        _attach(socket);
        socket->connectToHost(config.host, port);
        return {};
    }
    auto* socket = new QSslSocket(this);
    // No ALPN: NTRIP 1.0 casters do not speak HTTP/1.1, and a strict ALPN server may reject an unknown protocol.
    socket->setSslConfiguration(QGCNetworkHelper::createSslConfig());
    _attach(socket);
    socket->connectToHostEncrypted(config.host, port, config.host);
    return {};
}

void NTRIPHttpSession::_attach(QTcpSocket* socket)
{
    socket->setParent(this);
    _socket = socket;
    socket->setReadBufferSize(READ_BUFFER_BYTES);

    connect(socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError code) {
        // closed() follows once the remaining data is delivered.
        if (_ended || !_socket || code == QAbstractSocket::RemoteHostClosedError) {
            return;
        }
        const QString message = _socket->errorString();
        qCWarning(NTRIPHttpSessionLog) << "Socket error code:" << code << "msg:" << message;
        // Like a rejected certificate, a failed handshake or a missing TLS backend recurs on every attempt.
        const bool tls = code == QAbstractSocket::SslHandshakeFailedError || code == QAbstractSocket::SslInternalError;
        _fail(tls ? NTRIPError::SslError : NTRIPError::SocketError, message);
    });
    connect(socket, &QTcpSocket::readyRead, this, &NTRIPHttpSession::_read);
    connect(socket, &QTcpSocket::disconnected, this, &NTRIPHttpSession::_read);

    auto* sslSocket = qobject_cast<QSslSocket*>(socket);
    if (!sslSocket) {
        connect(socket, &QTcpSocket::connected, this, &NTRIPHttpSession::_establish);
        return;
    }
    connect(sslSocket, &QSslSocket::encrypted, this, &NTRIPHttpSession::_establish);
    connect(sslSocket, &QSslSocket::sslErrors, this,
            [this, sslSocket](const QList<QSslError>& errors) { _verifyCertificate(sslSocket, errors); });
}

void NTRIPHttpSession::_verifyCertificate(QSslSocket* socket, const QList<QSslError>& errors)
{
    if (_ended) {
        return;
    }
    QStringList messages;
    for (const QSslError& error : errors) {
        qCWarning(NTRIPHttpSessionLog) << "TLS error:" << error.errorString();
        messages.append(error.errorString());
    }
    const QSslCertificate certificate = socket->peerCertificate();
    if (!NTRIPTlsPolicy::isSelfSignedOnly(errors) || certificate.isNull()) {
        _fail(NTRIPError::SslError, messages.join(QStringLiteral("; ")));
        return;
    }
    if (!_allowSelfSignedCerts) {
        qCWarning(NTRIPHttpSessionLog)
            << "Rejecting self-signed certificate (enable 'Accept self-signed certificates' to allow)";
        _fail(NTRIPError::SslError, tr("Self-signed certificate rejected. Enable 'Accept self-signed "
                                       "certificates' in NTRIP settings to allow."));
        return;
    }
    const QString pin = NTRIPTlsPolicy::certificatePin(_endpoint, certificate);
    if (pin == _pinnedCertificate) {
        qCDebug(NTRIPHttpSessionLog) << "Accepting the pinned self-signed certificate";
    } else if (_pinnedCertificate.startsWith(_endpoint + QLatin1Char('|'))) {
        qCWarning(NTRIPHttpSessionLog) << "Rejecting self-signed certificate that differs from the pinned one";
        _fail(NTRIPError::SslError,
              tr("The caster's certificate changed since it was first trusted. If the change is expected, turn "
                 "'Accept self-signed certificates' off and on again to trust the new certificate."));
        return;
    } else {
        qCWarning(NTRIPHttpSessionLog) << "Accepting self-signed certificate (user opted in)";
        _newPin = pin;
    }
    // Only the reported self-signed errors are ignored; any other error remains fatal.
    socket->ignoreSslErrors(errors);
}

void NTRIPHttpSession::_establish()
{
    if (_ended || !_socket) {
        return;
    }
    qCDebug(NTRIPHttpSessionLog) << "Socket connected"
                                 << "local" << _socket->localAddress().toString() << ":" << _socket->localPort()
                                 << "-> peer" << _socket->peerAddress().toString() << ":" << _socket->peerPort();
    // Qt ignores socket options set before the native socket exists.
    _socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    _socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    if (const QString pin = std::exchange(_newPin, {}); !pin.isEmpty()) {
        const QPointer<NTRIPHttpSession> self(this);
        emit certificatePinned(pin);
        if (!self || _ended || !_socket) {
            return;
        }
    }
    if (!write(_request)) {
        _fail(NTRIPError::SocketError, tr("Socket did not accept the complete NTRIP write"));
        return;
    }
    qCDebug(NTRIPHttpSessionLog) << "HTTP request sent to" << _endpoint;
}

bool NTRIPHttpSession::write(const QByteArray& bytes)
{
    const auto socket = _socket;
    if (_ended || !socket) {
        return false;
    }
    return socket->write(bytes) == bytes.size();
}

bool NTRIPHttpSession::isConnected() const
{
    return !_ended && _socket && _socket->state() == QAbstractSocket::ConnectedState;
}

void NTRIPHttpSession::_read()
{
    const auto socket = _socket;
    if (_ended || _reading || !socket) {
        return;
    }
    const QPointer<NTRIPHttpSession> self(this);
    _reading = true;
    bool current = true;
    while (current && socket->bytesAvailable() > 0) {
        const qint64 receivedAtMs = _scheduler->nowMs();
        const QByteArray bytes = socket->read(READ_CHUNK_BYTES);
        if (bytes.isEmpty()) {
            break;
        }
        _receive(bytes, receivedAtMs);
        current = self && !_ended;
    }
    if (!self) {
        return;
    }
    _reading = false;
    if (current && socket->state() == QAbstractSocket::UnconnectedState) {
        _finishResponse();
    }
}

void NTRIPHttpSession::_receive(QByteArrayView bytes, qint64 receivedAtMs, const QDateTime& utcNow)
{
    if (!_ended) {
        _publish(_decoder.feed(bytes, utcNow), receivedAtMs);
    }
}

void NTRIPHttpSession::_publish(const NTRIPHttpDecoder::Result& result, qint64 receivedAtMs)
{
    // Observers may abort or retire this session from each signal.
    const QPointer<NTRIPHttpSession> self(this);
    if (result.connected) {
        emit responseStarted();
        if (!self || _ended) {
            return;
        }
    }
    if (!result.body.isEmpty()) {
        emit bodyReceived(result.body, receivedAtMs);
        if (!self || _ended) {
            return;
        }
    }
    if (result.failure) {
        _end();
        emit failed(*result.failure);
    } else if (result.complete) {
        _end();
        emit finished();
    } else if (_decoder.awaitingErrorBody() && !_errorBodyTask.active()) {
        _errorBodyTask.schedule(ERROR_BODY_TIMEOUT, [this]() { _finishResponse(); });
    }
}

void NTRIPHttpSession::_finishResponse()
{
    // A delivery in progress publishes the rest of the error body first.
    if (!_ended && !_reading) {
        _publish(_decoder.finish(), _scheduler->nowMs());
    }
}

void NTRIPHttpSession::_fail(NTRIPError code, const QString& message)
{
    if (_ended) {
        return;
    }
    // An HTTP error status explains the failure better than the event that cut its body short.
    if (_decoder.awaitingErrorBody()) {
        _publish(_decoder.finish(), _scheduler->nowMs());
        return;
    }
    _end();
    emit failed(NTRIPFailure{code, message});
}

void NTRIPHttpSession::_end()
{
    _ended = true;
    _errorBodyTask.cancel();
}

void NTRIPHttpSession::abort()
{
    _end();
    if (const auto socket = _socket) {
        socket->abort();
    }
}

void NTRIPHttpSession::retire()
{
    if (QObject* owner = parent()) {
        disconnect(owner);
        setParent(nullptr);
    }
    deleteLater();
    abort();
}
