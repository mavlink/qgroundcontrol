#include "NTRIPHttpSession.h"

#include <utility>

#include <QtCore/QPointer>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslError>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QTcpSocket>

#include "MonotonicClock.h"
#include "NTRIPConfiguration.h"
#include "NTRIPTlsPolicy_p.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPHttpSessionLog, "GPS.NTRIP.NTRIPHttpSession")

NTRIPHttpSession::NTRIPHttpSession(QObject* parent)
    : QObject(parent)
{}

void NTRIPHttpSession::open(const NTRIPConnectionConfig& config)
{
    if (_socket || _ended) {
        return;
    }
    const auto port = static_cast<quint16>(config.port);
    _endpoint = QStringLiteral("%1:%2").arg(config.host).arg(port);
    _pinnedCertificate = config.pinnedCertificate;
    _allowSelfSignedCerts = config.allowSelfSignedCerts;
    qCDebug(NTRIPHttpSessionLog) << "connectToHost" << config.host << ":" << config.port << "TLS" << config.useTls;
    if (!config.useTls) {
        auto* socket = new QTcpSocket(this);
        _attach(socket);
        socket->connectToHost(config.host, port);
        return;
    }
    auto* socket = new QSslSocket(this);
    // No ALPN: NTRIP 1.0 casters do not speak HTTP/1.1, and a strict ALPN server may reject an unknown protocol.
    QSslConfiguration tls = QSslConfiguration::defaultConfiguration();
    tls.setProtocol(QSsl::TlsV1_2OrLater);
    socket->setSslConfiguration(tls);
    _attach(socket);
    socket->connectToHostEncrypted(config.host, port, config.host);
}

void NTRIPHttpSession::_attach(QTcpSocket* socket)
{
    socket->setParent(this);
    _socket = socket;
    socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    socket->setReadBufferSize(kReadBufferBytes);

    connect(socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError code) {
        // closed() follows once the remaining data is delivered.
        if (_ended || !_socket || code == QAbstractSocket::RemoteHostClosedError) {
            return;
        }
        const QString message = _socket->errorString();
        qCWarning(NTRIPHttpSessionLog) << "Socket error code:" << code << "msg:" << message;
        _fail(NTRIPError::SocketError, message);
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
    if (const QString pin = std::exchange(_newPin, {}); !pin.isEmpty()) {
        const QPointer<NTRIPHttpSession> self(this);
        emit certificatePinned(pin);
        if (!self || _ended || !_socket) {
            return;
        }
    }
    emit established();
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
        const qint64 receivedAtMs = static_cast<qint64>(MonotonicClock::nowUs() / 1000);
        const QByteArray bytes = socket->read(kReadChunkBytes);
        if (bytes.isEmpty()) {
            break;
        }
        emit bytesReceived(bytes, receivedAtMs);
        current = self && !_ended;
    }
    if (!self) {
        return;
    }
    _reading = false;
    if (current && socket->state() == QAbstractSocket::UnconnectedState) {
        _ended = true;
        emit closed();
    }
}

void NTRIPHttpSession::_fail(NTRIPError code, const QString& message)
{
    if (!std::exchange(_ended, true)) {
        emit failed(code, message);
    }
}

void NTRIPHttpSession::abort()
{
    _ended = true;
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
