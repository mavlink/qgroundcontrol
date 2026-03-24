#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>

class TcpTransport;
class UdpTransport;

/// Encapsulates UDP/TCP/AutoFallback protocol selection, lazy transport
/// creation, TLS configuration, and TCP reconnection with exponential backoff.
class TransportStrategy : public QObject
{
    Q_OBJECT

public:
    enum class Protocol
    {
        UDP,
        TCP,
        AutoFallback
    };
    Q_ENUM(Protocol)

    explicit TransportStrategy(QObject* parent = nullptr);
    ~TransportStrategy() override;

    [[nodiscard]] Protocol protocol() const { return _protocol; }
    [[nodiscard]] QString host() const { return _host; }
    [[nodiscard]] quint16 port() const { return _port; }
    [[nodiscard]] bool tlsEnabled() const { return _tlsEnabled; }
    [[nodiscard]] bool tlsVerifyPeer() const { return _tlsVerifyPeer; }

    void setProtocol(Protocol protocol);
    void setTarget(const QString& host, quint16 port);

    void setTlsEnabled(bool enabled);
    void setTlsVerifyPeer(bool verify);
    void setTlsCaCertificates(const QList<QSslCertificate>& certs);
    void setTlsClientCertificate(const QSslCertificate& cert, const QSslKey& key);

    /// Send pre-formatted payload. Returns bytes sent (0 on failure/pending).
    quint64 send(const QByteArray& data);

    /// Whether the active transport is connected and ready.
    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] bool tcpConnected() const;

    /// Reset transports (e.g. after host/port change).
    void reset();

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& message);

private:
    void _ensureUdp();
    void _ensureTcp();
    void _configureTcp();
    bool _shouldUseTcp() const;

    void _onTcpConnected();
    void _onTcpDisconnected();
    void _onTransportError(const QString& message);

    Protocol _protocol = Protocol::UDP;
    QString _host;
    quint16 _port = 0;

    bool _tlsEnabled = false;
    bool _tlsVerifyPeer = true;
    QList<QSslCertificate> _tlsCaCerts;
    QSslCertificate _tlsClientCert;
    QSslKey _tlsClientKey;

    UdpTransport* _udpTransport = nullptr;
    TcpTransport* _tcpTransport = nullptr;

    int _tcpReconnectMs = kInitialReconnectMs;

    static constexpr int kUdpFailureThreshold = 10;
    static constexpr int kInitialReconnectMs = 5000;
    static constexpr int kMaxReconnectMs = 60000;
};
