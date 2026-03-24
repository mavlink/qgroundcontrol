#include "TransportStrategy.h"

#include "QGCLoggingCategory.h"
#include "RemoteTransport.h"

QGC_LOGGING_CATEGORY(TransportStrategyLog, "Utilities.TransportStrategy")

TransportStrategy::TransportStrategy(QObject* parent) : QObject(parent)
{
}

TransportStrategy::~TransportStrategy()
{
    if (_tcpTransport) {
        _tcpTransport->close();
    }
    if (_udpTransport) {
        _udpTransport->close();
    }
}

void TransportStrategy::setProtocol(Protocol protocol)
{
    if (_protocol == protocol) {
        return;
    }
    _protocol = protocol;
    if (_tcpTransport) {
        _tcpTransport->close();
    }
    if (_udpTransport) {
        _udpTransport->resetFailureCount();
    }
    _tcpReconnectMs = kInitialReconnectMs;
}

void TransportStrategy::setTarget(const QString& host, quint16 port)
{
    if (_host == host && _port == port) {
        return;
    }
    _host = host;
    _port = port;
    reset();
}

void TransportStrategy::reset()
{
    if (_tcpTransport) {
        _tcpTransport->close();
        _configureTcp();
    }
    if (_udpTransport) {
        _udpTransport->close();
        _udpTransport->deleteLater();
        _udpTransport = nullptr;
    }
    _tcpReconnectMs = kInitialReconnectMs;
}

// ---------------------------------------------------------------------------
// TLS configuration
// ---------------------------------------------------------------------------

void TransportStrategy::setTlsEnabled(bool enabled)
{
    _tlsEnabled = enabled;
    if (_tcpTransport) {
        _tcpTransport->close();
        _configureTcp();
    }
}

void TransportStrategy::setTlsVerifyPeer(bool verify)
{
    _tlsVerifyPeer = verify;
    if (_tcpTransport) {
        _tcpTransport->close();
        _configureTcp();
    }
}

void TransportStrategy::setTlsCaCertificates(const QList<QSslCertificate>& certs)
{
    _tlsCaCerts = certs;
    if (_tcpTransport) {
        _tcpTransport->setTlsCaCertificates(_tlsCaCerts);
    }
}

void TransportStrategy::setTlsClientCertificate(const QSslCertificate& cert, const QSslKey& key)
{
    _tlsClientCert = cert;
    _tlsClientKey = key;
    if (_tcpTransport) {
        _tcpTransport->setTlsClientCertificate(_tlsClientCert, _tlsClientKey);
    }
}

// ---------------------------------------------------------------------------
// Transport lifecycle
// ---------------------------------------------------------------------------

void TransportStrategy::_ensureUdp()
{
    if (!_udpTransport) {
        _udpTransport = new UdpTransport(this);
        _udpTransport->setTarget(_host, _port);
    }
}

void TransportStrategy::_ensureTcp()
{
    if (!_tcpTransport) {
        _tcpTransport = new TcpTransport(this);
        _configureTcp();
        (void)connect(_tcpTransport, &TcpTransport::connected, this, &TransportStrategy::_onTcpConnected);
        (void)connect(_tcpTransport, &TcpTransport::disconnected, this, &TransportStrategy::_onTcpDisconnected);
        (void)connect(_tcpTransport, &TcpTransport::errorOccurred, this, &TransportStrategy::_onTransportError);
    }
}

void TransportStrategy::_configureTcp()
{
    if (!_tcpTransport) {
        return;
    }
    _tcpTransport->setTarget(_host, _port);
    _tcpTransport->setTlsEnabled(_tlsEnabled);
    _tcpTransport->setTlsVerifyPeer(_tlsVerifyPeer);
    if (!_tlsCaCerts.isEmpty()) {
        _tcpTransport->setTlsCaCertificates(_tlsCaCerts);
    }
    if (!_tlsClientCert.isNull()) {
        _tcpTransport->setTlsClientCertificate(_tlsClientCert, _tlsClientKey);
    }
}

bool TransportStrategy::_shouldUseTcp() const
{
    if (_protocol == Protocol::TCP) {
        return true;
    }
    if (_protocol == Protocol::AutoFallback && _udpTransport &&
        _udpTransport->failureCount() >= kUdpFailureThreshold) {
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Send
// ---------------------------------------------------------------------------

quint64 TransportStrategy::send(const QByteArray& data)
{
    if (_host.isEmpty() || _port == 0) {
        return 0;
    }

    if (_shouldUseTcp()) {
        _ensureTcp();

        if (!_tcpTransport->isConnected()) {
            _tcpTransport->connectToHost();
            _tcpReconnectMs = qMin(_tcpReconnectMs * 2, kMaxReconnectMs);
            return 0;
        }

        if (!_tcpTransport->send(data)) {
            return 0;
        }
        // TCP framing adds 4 bytes length prefix
        return static_cast<quint64>(data.size()) + 4;
    }

    _ensureUdp();
    if (!_udpTransport->send(data)) {
        return 0;
    }
    return static_cast<quint64>(data.size());
}

bool TransportStrategy::isConnected() const
{
    if (_shouldUseTcp()) {
        return _tcpTransport && _tcpTransport->isConnected();
    }
    return _udpTransport != nullptr;
}

bool TransportStrategy::tcpConnected() const
{
    return _tcpTransport && _tcpTransport->isConnected();
}

// ---------------------------------------------------------------------------
// Transport callbacks
// ---------------------------------------------------------------------------

void TransportStrategy::_onTcpConnected()
{
    _tcpReconnectMs = kInitialReconnectMs;
    emit connected();
}

void TransportStrategy::_onTcpDisconnected()
{
    emit disconnected();
}

void TransportStrategy::_onTransportError(const QString& message)
{
    emit errorOccurred(message);
}
