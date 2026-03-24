#include "LogRemoteSink.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "LogEntry.h"
#include "LogFormatter.h"
#include "QGCCompression.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"

QGC_LOGGING_CATEGORY(LogRemoteSinkLog, "Utilities.LogRemoteSink")

LogRemoteSink::LogRemoteSink(QObject* parent) : QObject(parent), _transport(this)
{
    _batchTimer.setSingleShot(true);
    (void)connect(&_batchTimer, &QChronoTimer::timeout, this, &LogRemoteSink::_flushBatch);

    (void)connect(&_transport, &TransportStrategy::connected, this, [this]() { emit tcpConnectedChanged(); });
    (void)connect(&_transport, &TransportStrategy::disconnected, this, [this]() { emit tcpConnectedChanged(); });
    (void)connect(&_transport, &TransportStrategy::errorOccurred, this, [this](const QString& msg) {
        _setLastError(msg);
    });
}

LogRemoteSink::~LogRemoteSink()
{
    _flushBatch();
}

// ---------------------------------------------------------------------------
// Connection-relevant property setters
// ---------------------------------------------------------------------------

void LogRemoteSink::setEnabled(bool enabled)
{
    if (_enabled == enabled) {
        return;
    }
    _enabled = enabled;
    if (!_enabled) {
        _flushBatch();
        _transport.reset();
    }
    emit enabledChanged();
}

void LogRemoteSink::setHost(const QString& host)
{
    if (_transport.host() == host) {
        return;
    }
    _transport.setTarget(host, _transport.port());
    emit hostChanged();
}

void LogRemoteSink::setPort(quint16 port)
{
    if (_transport.port() == port) {
        return;
    }
    _transport.setTarget(_transport.host(), port);
    emit portChanged();
}

void LogRemoteSink::setProtocol(TransportStrategy::Protocol protocol)
{
    if (_transport.protocol() == protocol) {
        return;
    }
    _transport.setProtocol(protocol);
    emit protocolChanged();
}

void LogRemoteSink::setTlsEnabled(bool enabled)
{
    if (_transport.tlsEnabled() == enabled) {
        return;
    }
    _transport.setTlsEnabled(enabled);
    emit tlsEnabledChanged();
}

void LogRemoteSink::setTlsVerifyPeer(bool verify)
{
    if (_transport.tlsVerifyPeer() == verify) {
        return;
    }
    _transport.setTlsVerifyPeer(verify);
    emit tlsVerifyPeerChanged();
}

// ---------------------------------------------------------------------------
// Simple property setters
// ---------------------------------------------------------------------------

void LogRemoteSink::setVehicleId(const QString& id)
{
    if (_vehicleId == id) {
        return;
    }
    _vehicleId = id;
    emit vehicleIdChanged();
}

void LogRemoteSink::setCompressionEnabled(bool enabled)
{
    if (_compressionEnabled == enabled) {
        return;
    }
    _compressionEnabled = enabled;
    emit compressionEnabledChanged();
}

void LogRemoteSink::setCompressionLevel(int level)
{
    level = qBound(1, level, 9);
    if (_compressionLevel == level) {
        return;
    }
    _compressionLevel = level;
    emit compressionLevelChanged();
}

// ---------------------------------------------------------------------------
// TLS certificate management
// ---------------------------------------------------------------------------

void LogRemoteSink::resetBytesSent()
{
    _bytesSent = 0;
    emit bytesSentChanged();
}

bool LogRemoteSink::loadTlsCaCertificates(const QString& filePath)
{
    QString error;
    const auto certs = QGCNetworkHelper::loadCaCertificates(filePath, &error);
    if (certs.isEmpty()) {
        _setLastTlsError(error);
        return false;
    }
    _transport.setTlsCaCertificates(certs);
    return true;
}

bool LogRemoteSink::loadTlsClientCertificate(const QString& certPath, const QString& keyPath)
{
    QString error;
    QSslCertificate cert;
    QSslKey key;
    if (!QGCNetworkHelper::loadClientCertAndKey(certPath, keyPath, cert, key, &error)) {
        _setLastTlsError(error);
        return false;
    }
    _transport.setTlsClientCertificate(cert, key);
    return true;
}

void LogRemoteSink::setMaxPendingEntries(int max)
{
    max = qMax(10, max);
    if (_maxPendingEntries != max) {
        _maxPendingEntries = max;
        emit maxPendingEntriesChanged();
    }
}

// ---------------------------------------------------------------------------
// Entry formatting & compression
// ---------------------------------------------------------------------------

QByteArray LogRemoteSink::_formatEntry(const LogEntry& entry) const
{
    QJsonObject obj = LogFormatter::entryToJson(entry, LogFormatter::RemoteCompactSchema);
    if (!_vehicleId.isEmpty()) {
        obj[QStringLiteral("v")] = _vehicleId;
    }
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

QByteArray LogRemoteSink::_maybeCompress(const QByteArray& data) const
{
    if (!_compressionEnabled) {
        return data;
    }
    return QGCCompression::compress(data, static_cast<QGCCompression::CompressionLevel>(_compressionLevel));
}

// ---------------------------------------------------------------------------
// Batched sending
// ---------------------------------------------------------------------------

void LogRemoteSink::send(const LogEntry& entry)
{
    if (!_enabled || _transport.host().isEmpty() || _transport.port() == 0) {
        return;
    }

    if (_batch.size() >= _maxPendingEntries) {
        if (++_droppedEntries == 1 || (_droppedEntries % 100) == 0) {
            emit droppedEntriesChanged();
        }
        return;
    }

    _batch.append(_formatEntry(entry));
    if (_batch.size() >= kBatchSize) {
        _batchTimer.stop();
        _flushBatch();
    } else if (!_batchTimer.isActive()) {
        _batchTimer.start();
    }
}

void LogRemoteSink::_flushBatch()
{
    _batchTimer.stop();

    if (_batch.isEmpty()) {
        return;
    }

    QByteArray payload;
    if (_batch.size() == 1) {
        payload = _batch.first();
    } else {
        int totalSize = 2; // '[' + ']'
        for (const auto& b : std::as_const(_batch)) {
            totalSize += b.size() + 1; // entry + ','
        }
        payload.reserve(totalSize);
        payload.append('[');
        for (int i = 0; i < _batch.size(); ++i) {
            if (i > 0) {
                payload.append(',');
            }
            payload.append(_batch.at(i));
        }
        payload.append(']');
    }

    const QByteArray data = _maybeCompress(payload);
    const quint64 sent = _transport.send(data);

    if (sent > 0) {
        _bytesSent += sent;
        _batch.clear();
        emit bytesSentChanged();

        if (!_lastError.isEmpty()) {
            _lastError.clear();
            emit lastErrorChanged();
        }
    } else {
        // Transport not ready (e.g. TCP connecting) — keep batch for retry
        while (_batch.size() > _maxPendingEntries) {
            _batch.removeFirst();
            ++_droppedEntries;
        }
    }
}

// ---------------------------------------------------------------------------
// Error reporting
// ---------------------------------------------------------------------------

void LogRemoteSink::_setLastError(const QString& error)
{
    if (_lastError != error) {
        _lastError = error;
        emit lastErrorChanged();
        emit errorOccurred(error);
    }
}

void LogRemoteSink::_setLastTlsError(const QString& error)
{
    if (_lastTlsError != error) {
        _lastTlsError = error;
        emit lastTlsErrorChanged();
    }
}
