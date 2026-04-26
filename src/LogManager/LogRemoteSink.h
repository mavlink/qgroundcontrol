#pragma once

#include <QtCore/QChronoTimer>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>
#include <chrono>

#include "TransportStrategy.h"

struct LogEntry;

class LogRemoteSink : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QString vehicleId READ vehicleId WRITE setVehicleId NOTIFY vehicleIdChanged)

    Q_PROPERTY(TransportStrategy::Protocol protocol READ protocol WRITE setProtocol NOTIFY protocolChanged)
    Q_PROPERTY(bool tcpConnected READ tcpConnected NOTIFY tcpConnectedChanged)

    Q_PROPERTY(bool tlsEnabled READ tlsEnabled WRITE setTlsEnabled NOTIFY tlsEnabledChanged)
    Q_PROPERTY(bool tlsVerifyPeer READ tlsVerifyPeer WRITE setTlsVerifyPeer NOTIFY tlsVerifyPeerChanged)

    Q_PROPERTY(
        bool compressionEnabled READ compressionEnabled WRITE setCompressionEnabled NOTIFY compressionEnabledChanged)
    Q_PROPERTY(int compressionLevel READ compressionLevel WRITE setCompressionLevel NOTIFY compressionLevelChanged)

    Q_PROPERTY(quint64 bytesSent READ bytesSent NOTIFY bytesSentChanged)
    Q_PROPERTY(quint64 droppedEntries READ droppedEntries NOTIFY droppedEntriesChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString lastTlsError READ lastTlsError NOTIFY lastTlsErrorChanged)
    Q_PROPERTY(int maxPendingEntries READ maxPendingEntries WRITE setMaxPendingEntries NOTIFY maxPendingEntriesChanged)

public:
    // Re-export for QML access (TransportStrategy isn't a QML type)
    enum RemoteProtocol
    {
        UDP = static_cast<int>(TransportStrategy::Protocol::UDP),
        TCP = static_cast<int>(TransportStrategy::Protocol::TCP),
        AutoFallback = static_cast<int>(TransportStrategy::Protocol::AutoFallback),
    };
    Q_ENUM(RemoteProtocol)
    static_assert(AutoFallback == 2, "RemoteProtocol out of sync with TransportStrategy::Protocol");

    explicit LogRemoteSink(QObject* parent = nullptr);
    ~LogRemoteSink() override;

    bool enabled() const { return _enabled; }

    void setEnabled(bool enabled);

    // Delegated to TransportStrategy — single source of truth
    QString host() const { return _transport.host(); }

    void setHost(const QString& host);

    quint16 port() const { return _transport.port(); }

    void setPort(quint16 port);

    QString vehicleId() const { return _vehicleId; }

    void setVehicleId(const QString& id);

    TransportStrategy::Protocol protocol() const { return _transport.protocol(); }

    void setProtocol(TransportStrategy::Protocol protocol);

    bool tcpConnected() const { return _transport.tcpConnected(); }

    bool tlsEnabled() const { return _transport.tlsEnabled(); }

    void setTlsEnabled(bool enabled);

    bool tlsVerifyPeer() const { return _transport.tlsVerifyPeer(); }

    void setTlsVerifyPeer(bool verify);

    bool compressionEnabled() const { return _compressionEnabled; }

    void setCompressionEnabled(bool enabled);

    int compressionLevel() const { return _compressionLevel; }

    void setCompressionLevel(int level);

    quint64 bytesSent() const { return _bytesSent; }

    quint64 droppedEntries() const { return _droppedEntries; }

    QString lastError() const { return _lastError; }

    QString lastTlsError() const { return _lastTlsError; }

    Q_INVOKABLE void resetBytesSent();
    Q_INVOKABLE bool loadTlsCaCertificates(const QString& filePath);
    Q_INVOKABLE bool loadTlsClientCertificate(const QString& certPath, const QString& keyPath);

    int maxPendingEntries() const { return _maxPendingEntries; }

    void setMaxPendingEntries(int max);

    void send(const LogEntry& entry);

signals:
    void enabledChanged();
    void hostChanged();
    void portChanged();
    void vehicleIdChanged();
    void protocolChanged();
    void tcpConnectedChanged();
    void tlsEnabledChanged();
    void tlsVerifyPeerChanged();
    void compressionEnabledChanged();
    void compressionLevelChanged();
    void bytesSentChanged();
    void lastErrorChanged();
    void lastTlsErrorChanged();
    void droppedEntriesChanged();
    void maxPendingEntriesChanged();
    void errorOccurred(const QString& message);

private slots:
    void _flushBatch();

private:
    void _setLastError(const QString& error);
    void _setLastTlsError(const QString& error);
    QByteArray _formatEntry(const LogEntry& entry) const;
    QByteArray _maybeCompress(const QByteArray& data) const;

    bool _enabled = false;
    QString _vehicleId;

    bool _compressionEnabled = false;
    int _compressionLevel = 6;

    quint64 _bytesSent = 0;
    quint64 _droppedEntries = 0;
    QString _lastError;
    QString _lastTlsError;

    TransportStrategy _transport;
    QChronoTimer _batchTimer{std::chrono::milliseconds{kBatchIntervalMs}};
    QList<QByteArray> _batch;
    int _maxPendingEntries = 1000;

    static constexpr int kBatchSize = 50;
    static constexpr int kBatchIntervalMs = 200;
};
