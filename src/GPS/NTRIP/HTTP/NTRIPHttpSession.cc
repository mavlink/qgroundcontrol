#include "NTRIPHttpSession.h"

#include <algorithm>
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

NTRIPVersion NTRIPCasterVersions::versionFor(const NTRIPConnectionConfig& caster) const
{
    return _version1Casters.contains(caster.url(false).toString()) ? NTRIPVersion::Version1 : NTRIPVersion::Version2;
}

void NTRIPCasterVersions::setVersion(const NTRIPConnectionConfig& caster, NTRIPVersion version)
{
    const QString endpoint = caster.url(false).toString();
    QStringList casters = _version1Casters;
    casters.removeAll(endpoint);
    if (version == NTRIPVersion::Version1) {
        casters.append(endpoint);
    }
    _remember(casters);
}

QString NTRIPCasterVersions::saved() const
{
    return _version1Casters.join(QLatin1Char(' '));
}

void NTRIPCasterVersions::restore(const QString& saved)
{
    QStringList casters;
    for (const QString& caster : saved.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        // Named again later, a caster is a later one.
        casters.removeAll(caster);
        casters.append(caster);
    }
    _remember(casters);
}

void NTRIPCasterVersions::_remember(const QStringList& casters)
{
    const QStringList latest = casters.last((std::min) (casters.size(), MAX_CASTERS));
    if (latest != _version1Casters) {
        _version1Casters = latest;
        emit changed();
    }
}

NTRIPHttpSession::NTRIPHttpSession(QObject* parent, RuntimeScheduler* scheduler, NTRIPHttpPurpose purpose)
    : QObject(parent)
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
    , _purpose(purpose)
    , _errorBodyTask(_scheduler, this)
    , _decoder(purpose)
{}

QString NTRIPHttpSession::open(const NTRIPConnectionConfig& config, std::shared_ptr<NTRIPCasterVersions> casterVersions)
{
    if (_socket || _ended) {
        return {};
    }
    const NTRIPVersion version = casterVersions ? casterVersions->versionFor(config) : NTRIPVersion::Version2;
    NTRIPHttpRequest request = NTRIPHttpRequest::build(config, _purpose, version);
    if (!request.error.isEmpty()) {
        return request.error;
    }
    _request = std::move(request.bytes);
    _version = version;
    _config = config;
    _casterVersions = std::move(casterVersions);
    _endpoint = config.endpoint();
    _connect();
    return {};
}

void NTRIPHttpSession::_connect()
{
    const auto port = static_cast<quint16>(_config.port);
    qCDebug(NTRIPHttpSessionLog) << "connectToHost" << _config.host << ":" << _config.port << "TLS" << _config.useTls;
    if (!_config.useTls) {
        auto* socket = new QTcpSocket(this);
        _attach(socket);
        socket->connectToHost(_config.host, port);
        return;
    }
    auto* socket = new QSslSocket(this);
    // No ALPN: NTRIP 1.0 casters do not speak HTTP/1.1, and a strict ALPN server may reject an unknown protocol.
    socket->setSslConfiguration(QGCNetworkHelper::createSslConfig());
    _attach(socket);
    socket->connectToHostEncrypted(_config.host, port, _config.host);
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
    if (!_config.allowSelfSignedCerts) {
        qCWarning(NTRIPHttpSessionLog)
            << "Rejecting self-signed certificate (enable 'Accept self-signed certificates' to allow)";
        _fail(NTRIPError::SslError, tr("Self-signed certificate rejected. Enable 'Accept self-signed "
                                       "certificates' in NTRIP settings to allow."));
        return;
    }
    const QString pin = NTRIPTlsPolicy::certificatePin(_endpoint, certificate);
    if (pin == _config.pinnedCertificate) {
        qCDebug(NTRIPHttpSessionLog) << "Accepting the pinned self-signed certificate";
    } else if (_config.pinnedCertificate.startsWith(_endpoint + QLatin1Char('|'))) {
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
    // A stream that waits for a position carries no traffic and has no stall timeout, and the system's default is to
    // probe after two hours: find a dead link after about a minute instead.
    _socket->setSocketOption(QAbstractSocket::KeepAliveIdleOption, 30);
    _socket->setSocketOption(QAbstractSocket::KeepAliveIntervalOption, 10);
    _socket->setSocketOption(QAbstractSocket::KeepAliveCountOption, 3);
    _socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    if (const QString pin = std::exchange(_newPin, {}); !pin.isEmpty()) {
        // A repeated request finds the certificate pinned.
        _config.pinnedCertificate = pin;
        const QPointer<NTRIPHttpSession> self(this);
        emit certificatePinned(pin);
        if (!self || _ended || !_socket) {
            return;
        }
    }
    if (_socket->write(_request) != _request.size()) {
        _fail(NTRIPError::SocketError, tr("Socket did not accept the complete NTRIP write"));
        return;
    }
    _requestSent = true;
    qCDebug(NTRIPHttpSessionLog) << "HTTP request sent to" << _endpoint;
}

bool NTRIPHttpSession::write(const QByteArray& bytes)
{
    return isConnected() && _socket->write(bytes) == bytes.size();
}

bool NTRIPHttpSession::isConnected() const
{
    // A TLS socket is connected, and queues what is written to it, from before its handshake; the request goes first.
    return !_ended && _requestSent && _socket && _socket->state() == QAbstractSocket::ConnectedState;
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
        // Before they are decoded, which may end the exchange, as an observer of this may.
        emit dataReceived(bytes.size());
        current = self && !_ended;
        if (!current) {
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
        _closed();
    }
}

void NTRIPHttpSession::_closed()
{
    // A close and a reset arrive alike. A caster that sent anything has read the request, so asking again in another
    // form would only count against the client.
    if (_requestSent && !_answered && _version == NTRIPVersion::Version2) {
        _repeatInVersion1();
        return;
    }
    _finishResponse();
}

void NTRIPHttpSession::_repeatInVersion1()
{
    qCWarning(NTRIPHttpSessionLog) << "Caster closed the connection without answering: "
                                      "repeating the request in NTRIP version 1 form";
    _version = NTRIPVersion::Version1;
    // The configuration built the first request, so it builds this one.
    _request = NTRIPHttpRequest::build(_config, _purpose, _version).bytes;
    _requestSent = false;
    if (QTcpSocket* const closed = std::exchange(_socket, nullptr)) {
        closed->disconnect(this);
        closed->deleteLater();
    }
    _connect();
}

void NTRIPHttpSession::_receive(QByteArrayView bytes, qint64 receivedAtMs, const QDateTime& utcNow)
{
    if (!_ended) {
        _answered = true;
        _publish(_decoder.feed(bytes, utcNow), receivedAtMs);
    }
}

void NTRIPHttpSession::_publish(const NTRIPHttpDecoder::Result& result, qint64 receivedAtMs)
{
    // Observers may abort or retire this session from each signal.
    const QPointer<NTRIPHttpSession> self(this);
    if (result.connected) {
        _accepted = true;
        // The form the caster accepted is the one to ask it in next.
        if (_casterVersions) {
            _casterVersions->setVersion(_config, _version);
        }
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
        // A caster that refuses a version 1 request, or closes it unanswered, may take the version 2 form by now.
        // Saved, it would otherwise be asked in version 1 form for ever.
        if (_casterVersions && _version == NTRIPVersion::Version1 && !_accepted) {
            _casterVersions->setVersion(_config, NTRIPVersion::Version2);
        }
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
