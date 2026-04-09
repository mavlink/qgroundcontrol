#include "RemoteTransport.h"

#include <QtCore/QtEndian>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QUdpSocket>

// --- UdpTransport ---

UdpTransport::UdpTransport(QObject *parent)
    : RemoteTransport(parent)
{
}

void UdpTransport::setTarget(const QString &host, quint16 port)
{
    _host = host;
    _port = port;
}

bool UdpTransport::send(const QByteArray &data)
{
    if (!_socket) {
        _socket = new QUdpSocket(this);
    }

    if (data.size() > kMaxDatagramSize) {
        emit errorOccurred(tr("UDP payload too large (%1 bytes), dropped").arg(data.size()));
        return false;
    }

    QHostAddress addr(_host);
    if (addr.isNull()) {
        // Resolve hostname to IP
        const QHostInfo info = QHostInfo::fromName(_host);
        if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) {
            emit errorOccurred(tr("DNS resolution failed for '%1': %2").arg(_host, info.errorString()));
            return false;
        }
        addr = info.addresses().first();
    }
    const qint64 written = _socket->writeDatagram(data, addr, _port);
    if (written < 0) {
        ++_failureCount;
        emit errorOccurred(_socket->errorString());
        return false;
    }
    return true;
}

void UdpTransport::close()
{
    if (_socket) {
        _socket->deleteLater();
        _socket = nullptr;
    }
    _failureCount = 0;
}

// --- TcpTransport ---

TcpTransport::TcpTransport(QObject *parent)
    : RemoteTransport(parent)
{
}

void TcpTransport::setTarget(const QString &host, quint16 port)
{
    _host = host;
    _port = port;
}

void TcpTransport::setTlsEnabled(bool enabled) { _tlsEnabled = enabled; }
void TcpTransport::setTlsVerifyPeer(bool verify) { _tlsVerifyPeer = verify; }
void TcpTransport::setTlsCaCertificates(const QList<QSslCertificate> &certs) { _caCertificates = certs; }
void TcpTransport::setTlsClientCertificate(const QSslCertificate &cert, const QSslKey &key)
{
    _clientCert = cert;
    _clientKey = key;
}

bool TcpTransport::send(const QByteArray &data)
{
    if (!_socket || !_connected) {
        return false;
    }

    QByteArray frame;
    frame.reserve(4 + data.size());
    const quint32 len = qToBigEndian(static_cast<quint32>(data.size()));
    frame.append(reinterpret_cast<const char *>(&len), 4);
    frame.append(data);

    const qint64 written = _socket->write(frame);
    if (written < 0) {
        emit errorOccurred(_socket->errorString());
        return false;
    }
    return true;
}

void TcpTransport::close()
{
    if (_socket) {
        _socket->disconnect(this);
        _socket->disconnectFromHost();
        _socket->deleteLater();
        _socket = nullptr;
    }
    if (_connected) {
        _connected = false;
        emit disconnected();
    }
}

void TcpTransport::connectToHost()
{
    if (_socket && _socket->state() != QAbstractSocket::UnconnectedState) {
        return;
    }

    _createSocket();

    if (_tlsEnabled) {
        _socket->connectToHostEncrypted(_host, _port);
    } else {
        _socket->connectToHost(_host, _port);
    }
}

void TcpTransport::_createSocket()
{
    if (_socket) {
        _socket->deleteLater();
        _socket = nullptr;
    }

    _socket = new QSslSocket(this);

    if (_tlsEnabled) {
        QSslConfiguration config = QSslConfiguration::defaultConfiguration();
        if (!_caCertificates.isEmpty()) {
            config.setCaCertificates(_caCertificates);
        }
        if (!_clientCert.isNull()) {
            config.setLocalCertificate(_clientCert);
            config.setPrivateKey(_clientKey);
        }
        if (!_tlsVerifyPeer) {
            config.setPeerVerifyMode(QSslSocket::VerifyNone);
        }
        _socket->setSslConfiguration(config);
        connect(_socket, &QSslSocket::sslErrors, this, &TcpTransport::_onSslErrors);
    }

    connect(_socket, &QAbstractSocket::connected, this, &TcpTransport::_onConnected);
    connect(_socket, &QAbstractSocket::disconnected, this, &TcpTransport::_onDisconnected);
    connect(_socket, &QAbstractSocket::errorOccurred, this, &TcpTransport::_onError);
}

void TcpTransport::_onConnected()
{
    _connected = true;
    emit connected();
}

void TcpTransport::_onDisconnected()
{
    _connected = false;
    emit disconnected();
}

void TcpTransport::_onError(QAbstractSocket::SocketError)
{
    if (_socket) {
        emit errorOccurred(_socket->errorString());
    }
}

void TcpTransport::_onSslErrors(const QList<QSslError> &errors)
{
    QStringList messages;
    for (const auto &err : errors) {
        messages.append(err.errorString());
    }
    emit errorOccurred(QStringLiteral("TLS: ") + messages.join(QStringLiteral("; ")));

    if (!_tlsVerifyPeer && _socket) {
        // Only ignore certificate-related errors, not protocol/cipher failures
        QList<QSslError> certErrors;
        for (const auto &err : errors) {
            switch (err.error()) {
            case QSslError::SelfSignedCertificate:
            case QSslError::SelfSignedCertificateInChain:
            case QSslError::CertificateUntrusted:
            case QSslError::CertificateNotYetValid:
            case QSslError::CertificateExpired:
            case QSslError::HostNameMismatch:
            case QSslError::UnableToGetLocalIssuerCertificate:
            case QSslError::UnableToVerifyFirstCertificate:
                certErrors.append(err);
                break;
            default:
                break;
            }
        }
        if (!certErrors.isEmpty()) {
            _socket->ignoreSslErrors(certErrors);
        }
    }
}
