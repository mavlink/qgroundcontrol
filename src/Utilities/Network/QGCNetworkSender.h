#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QFuture>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QWaitCondition>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>

/// Async network sender supporting UDP, TCP, and TLS.
/// Uses QtConcurrent for non-blocking operation.
class QGCNetworkSender : public QObject
{
    Q_OBJECT

public:
    enum class Protocol {
        UDP,
        TCP,
        TLS
    };
    Q_ENUM(Protocol)

    explicit QGCNetworkSender(QObject* parent = nullptr);
    ~QGCNetworkSender() override;

    /// Sets the remote endpoint.
    void setEndpoint(const QHostAddress& host, quint16 port);
    QHostAddress host() const;
    quint16 port() const;

    /// Sets the transport protocol.
    void setProtocol(Protocol protocol);
    Protocol protocol() const;

    /// Starts the sender.
    void start();

    /// Stops the sender.
    void stop();

    /// Returns true if the sender is running.
    bool isRunning() const;

    /// Queues data for sending. Thread-safe, non-blocking.
    void send(const QByteArray& data);

    /// Forces immediate send of pending data.
    void flush();

    /// Returns true if TCP/TLS is currently connected.
    bool isConnected() const;

    /// Returns true if an error has occurred.
    bool hasError() const;

    /// Clears the error state.
    void clearError();

    /// Returns true if the last send operation succeeded.
    bool lastSendSucceeded() const;

    // Configuration
    void setMaxPendingBytes(qint64 bytes);
    qint64 maxPendingBytes() const;

    void setConnectTimeout(int ms);
    int connectTimeout() const;

    void setReconnectInterval(int ms);
    int reconnectInterval() const;

    // TLS configuration
    void setTlsVerifyPeer(bool verify);
    bool tlsVerifyPeer() const;

    void setTlsCaCertificates(const QList<QSslCertificate>& certs);
    void setTlsClientCertificate(const QSslCertificate& cert, const QSslKey& key);

    bool loadTlsCaCertificates(const QString& filePath);
    bool loadTlsClientCertificate(const QString& certPath, const QString& keyPath);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& message);
    void errorCleared();
    void tlsErrorOccurred(const QString& message);
    void dataSent(qint64 bytes);

private:
    void _senderLoop();
    bool _connectTcp();
    bool _connectTls();
    void _disconnect();
    bool _sendUdp(const QByteArray& data);
    bool _sendTcp(const QByteArray& data);
    void _handleSslErrors(const QList<QSslError>& errors);
    void _emitError(const QString& message);

    // Processing (QtConcurrent)
    QFuture<void> _future;
    mutable QMutex _mutex;
    QWaitCondition _cv;
    QQueue<QByteArray> _queue;
    bool _running = false;
    bool _flushRequested = false;

    // Network state (accessed from sender thread)
    QUdpSocket* _udpSocket = nullptr;
    QTcpSocket* _tcpSocket = nullptr;
    QSslSocket* _sslSocket = nullptr;

    // Endpoint (protected by _mutex)
    QHostAddress _host;
    quint16 _port = 0;

    // State (protected by _mutex)
    Protocol _protocol = Protocol::UDP;
    bool _connected = false;
    bool _hasError = false;
    bool _lastSendSucceeded = true;
    qint64 _pendingBytes = 0;
    QElapsedTimer _lastConnectAttempt;

    // Configuration (protected by _mutex)
    qint64 _maxPendingBytes = 10LL * 1024 * 1024; // 10MB
    int _connectTimeout = 3000;
    int _reconnectInterval = 5000;

    // TLS (protected by _mutex)
    bool _tlsVerifyPeer = true;
    QSslConfiguration _sslConfig;

    static constexpr int kSendTimeoutMs = 100;
};
