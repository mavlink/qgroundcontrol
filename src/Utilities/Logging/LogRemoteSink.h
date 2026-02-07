#pragma once

#include "LogEntry.h"

#include <QtCore/QFuture>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QWaitCondition>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>

class QGCNetworkSender;

/// Sends log entries to a remote server via UDP with optional TCP fallback.
/// Uses QGCNetworkSender for network I/O and handles batching, compression, and formatting.
class LogRemoteSink : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString endpoint READ endpoint WRITE setEndpoint NOTIFY endpointChanged)
    Q_PROPERTY(QString vehicleId READ vehicleId WRITE setVehicleId NOTIFY vehicleIdChanged)
    Q_PROPERTY(Protocol protocol READ protocol WRITE setProtocol NOTIFY protocolChanged)
    Q_PROPERTY(bool tcpConnected READ isTcpConnected NOTIFY tcpConnectedChanged)
    Q_PROPERTY(bool tlsEnabled READ isTlsEnabled WRITE setTlsEnabled NOTIFY tlsEnabledChanged)
    Q_PROPERTY(bool tlsVerifyPeer READ tlsVerifyPeer WRITE setTlsVerifyPeer NOTIFY tlsVerifyPeerChanged)
    Q_PROPERTY(bool compressionEnabled READ isCompressionEnabled WRITE setCompressionEnabled NOTIFY compressionEnabledChanged)
    Q_PROPERTY(int compressionLevel READ compressionLevel WRITE setCompressionLevel NOTIFY compressionLevelChanged)

public:
    enum Protocol {
        UDP,            ///< UDP only (fire-and-forget)
        TCP,            ///< TCP only (reliable, connection-based)
        AutoFallback    ///< Start with UDP, fall back to TCP on failures
    };
    Q_ENUM(Protocol)

    explicit LogRemoteSink(QObject* parent = nullptr);
    ~LogRemoteSink() override;

    /// Queues an entry for sending. Thread-safe, non-blocking.
    void send(const LogEntry& entry);

    /// Sets the remote endpoint (host:port format).
    void setEndpoint(const QString& hostPort);
    QString endpoint() const;

    /// Sets the vehicle identifier included in each log message.
    void setVehicleId(const QString& id);
    QString vehicleId() const;

    /// Enables or disables remote logging.
    void setEnabled(bool enabled);
    bool isEnabled() const;

    /// Sets the transport protocol.
    void setProtocol(Protocol protocol);
    Protocol protocol() const;

    /// Returns true if TCP is currently connected.
    bool isTcpConnected() const;

    /// Configuration
    void setMaxPendingEntries(int count);
    int maxPendingEntries() const;

    /// Sets the maximum number of entries per UDP datagram (batch size).
    void setBatchSize(int size);
    int batchSize() const;

    /// Sets the maximum datagram size in bytes. Batches exceeding this are split.
    void setMaxDatagramSize(int bytes);
    int maxDatagramSize() const;

    /// Sets the number of consecutive UDP failures before falling back to TCP (AutoFallback mode).
    void setUdpFailureThreshold(int count);
    int udpFailureThreshold() const;

    /// Sets the TCP connection timeout in milliseconds.
    void setTcpConnectTimeout(int ms);
    int tcpConnectTimeout() const;

    /// Sets the TCP reconnect interval in milliseconds.
    void setTcpReconnectInterval(int ms);
    int tcpReconnectInterval() const;

    /// Enables or disables TLS encryption for TCP connections.
    void setTlsEnabled(bool enabled);
    bool isTlsEnabled() const;

    /// Sets whether to verify peer certificates (default: true).
    void setTlsVerifyPeer(bool verify);
    bool tlsVerifyPeer() const;

    /// Sets CA certificates for peer verification.
    void setTlsCaCertificates(const QList<QSslCertificate>& certs);

    /// Sets client certificate for mutual TLS authentication.
    void setTlsClientCertificate(const QSslCertificate& cert, const QSslKey& key);

    /// Loads CA certificates from a PEM file.
    bool loadTlsCaCertificates(const QString& filePath);

    /// Loads client certificate and key from PEM files.
    bool loadTlsClientCertificate(const QString& certPath, const QString& keyPath);

    /// Enables or disables zlib compression.
    void setCompressionEnabled(bool enabled);
    bool isCompressionEnabled() const;

    /// Sets the zlib compression level (1-9, where 1=fastest, 9=best compression).
    void setCompressionLevel(int level);
    int compressionLevel() const;

    /// Sets the minimum payload size for compression. Messages smaller than this are sent uncompressed.
    void setMinCompressSize(int bytes);
    int minCompressSize() const;

signals:
    void enabledChanged();
    void endpointChanged();
    void vehicleIdChanged();
    void protocolChanged();
    void tcpConnectedChanged();
    void tlsEnabledChanged();
    void tlsVerifyPeerChanged();
    void tlsErrorOccurred(const QString& message);
    void compressionEnabledChanged();
    void compressionLevelChanged();
    void errorOccurred(const QString& message);
    void dataSent(qint64 bytes);

private slots:
    void _onSenderConnected();
    void _onSenderDisconnected();
    void _onDataSent(qint64 bytes);

private:
    void _startProcessing();
    void _stopProcessing();
    void _processingLoop();
    QByteArray _formatBatch(const QQueue<LogEntry>& entries, int startIdx, int count) const;
    QJsonObject _entryToJson(const LogEntry& entry) const;
    void _updateSenderProtocol();
    void _handleUdpFailure();

    // Network sender
    QGCNetworkSender* _udpSender = nullptr;
    QGCNetworkSender* _tcpSender = nullptr;

    // Processing (QtConcurrent)
    QFuture<void> _future;
    mutable QMutex _mutex;
    QWaitCondition _cv;
    QQueue<LogEntry> _queue;
    bool _running = false;

    // Endpoint (protected by _mutex)
    QString _endpoint;
    QHostAddress _host;
    quint16 _port = 0;
    QString _vehicleId;

    // Configuration (protected by _mutex)
    bool _enabled = false;
    int _maxPendingEntries = 1000;
    int _batchSize = 50;
    int _maxDatagramSize = 8192;
    Protocol _protocol = UDP;
    int _udpFailureThreshold = 5;

    // State (protected by _mutex)
    bool _usingTcpFallback = false;
    int _consecutiveUdpFailures = 0;

    // TLS (protected by _mutex)
    bool _tlsEnabled = false;
    bool _tlsVerifyPeer = true;

    // Compression (protected by _mutex)
    bool _compressionEnabled = false;
    int _compressionLevel = 6;
    int _minCompressSize = 256;

    static constexpr int kProcessTimeoutMs = 100;
};
