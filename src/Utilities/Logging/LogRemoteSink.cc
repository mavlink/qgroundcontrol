#include "LogRemoteSink.h"
#include "LogCompression.h"
#include <QtCore/QLoggingCategory>
#include "QGCNetworkSender.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QHostInfo>

Q_STATIC_LOGGING_CATEGORY(LogRemoteSinkLog, "Utilities.Logging.LogRemoteSink", QtWarningMsg)

LogRemoteSink::LogRemoteSink(QObject* parent)
    : QObject(parent)
    , _udpSender(new QGCNetworkSender(this))
    , _tcpSender(new QGCNetworkSender(this))
{
    _udpSender->setProtocol(QGCNetworkSender::Protocol::UDP);
    _tcpSender->setProtocol(QGCNetworkSender::Protocol::TCP);

    connect(_tcpSender, &QGCNetworkSender::connected, this, &LogRemoteSink::_onSenderConnected);
    connect(_tcpSender, &QGCNetworkSender::disconnected, this, &LogRemoteSink::_onSenderDisconnected);
    connect(_tcpSender, &QGCNetworkSender::tlsErrorOccurred, this, &LogRemoteSink::tlsErrorOccurred);
    connect(_tcpSender, &QGCNetworkSender::errorOccurred, this, &LogRemoteSink::errorOccurred);
    connect(_udpSender, &QGCNetworkSender::errorOccurred, this, &LogRemoteSink::errorOccurred);

    connect(_udpSender, &QGCNetworkSender::dataSent, this, &LogRemoteSink::_onDataSent);
    connect(_tcpSender, &QGCNetworkSender::dataSent, this, &LogRemoteSink::_onDataSent);
}

LogRemoteSink::~LogRemoteSink()
{
    _stopProcessing();
}

void LogRemoteSink::setEndpoint(const QString& hostPort)
{
    QHostAddress host;
    quint16 port = 0;
    bool valid = false;
    QString parseError;

    {
        QMutexLocker locker(&_mutex);

        if (_endpoint == hostPort) {
            return;
        }

        _endpoint = hostPort;
    }

    // Parse host:port with IPv6 bracket support (e.g., [::1]:5000)
    QString hostStr;
    QString portStr;

    if (hostPort.startsWith('[')) {
        const int bracketEnd = hostPort.indexOf(']');
        if (bracketEnd > 0 && bracketEnd + 1 < hostPort.size() && hostPort[bracketEnd + 1] == ':') {
            hostStr = hostPort.mid(1, bracketEnd - 1);
            portStr = hostPort.mid(bracketEnd + 2);
        }
    } else {
        const int colonIdx = hostPort.lastIndexOf(':');
        if (colonIdx > 0) {
            hostStr = hostPort.left(colonIdx);
            portStr = hostPort.mid(colonIdx + 1);
        }
    }

    bool portOk = false;
    const quint16 parsedPort = portStr.toUShort(&portOk);
    if (hostPort.trimmed().isEmpty()) {
        parseError.clear();
    } else if (hostStr.isEmpty() || !portOk || parsedPort == 0) {
        parseError = QStringLiteral("Invalid endpoint format: %1").arg(hostPort);
    } else {
        QHostAddress parsedHost(hostStr);
        if (parsedHost.isNull()) {
            const QHostInfo hostInfo = QHostInfo::fromName(hostStr);
            if (hostInfo.error() == QHostInfo::NoError) {
                for (const QHostAddress& addr : hostInfo.addresses()) {
                    if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
                        parsedHost = addr;
                        break;
                    }
                    if (parsedHost.isNull()) {
                        parsedHost = addr;
                    }
                }
            }
        }

        if (!parsedHost.isNull()) {
            host = parsedHost;
            port = parsedPort;
            valid = true;
        } else {
            parseError = QStringLiteral("Unable to resolve endpoint host: %1").arg(hostStr);
        }
    }

    {
        QMutexLocker locker(&_mutex);
        if (valid) {
            _host = host;
            _port = port;
            qCDebug(LogRemoteSinkLog) << "Endpoint set to" << _host.toString() << ":" << _port;
        } else {
            _host.clear();
            _port = 0;
            if (!parseError.isEmpty()) {
                qCWarning(LogRemoteSinkLog) << parseError;
            }
        }
    }

    // Update senders
    _udpSender->setEndpoint(host, port);
    _tcpSender->setEndpoint(host, port);

    if (!valid && !parseError.isEmpty()) {
        emit errorOccurred(parseError);
    }

    emit endpointChanged();
}

QString LogRemoteSink::endpoint() const
{
    QMutexLocker locker(&_mutex);
    return _endpoint;
}

void LogRemoteSink::setVehicleId(const QString& id)
{
    {
        QMutexLocker locker(&_mutex);

        if (_vehicleId == id) {
            return;
        }

        _vehicleId = id;
        qCDebug(LogRemoteSinkLog) << "Vehicle ID set to:" << id;
    }

    emit vehicleIdChanged();
}

QString LogRemoteSink::vehicleId() const
{
    QMutexLocker locker(&_mutex);
    return _vehicleId;
}

void LogRemoteSink::setEnabled(bool enabled)
{
    {
        QMutexLocker locker(&_mutex);
        if (_enabled == enabled) {
            return;
        }
        _enabled = enabled;

        if (enabled) {
            _consecutiveUdpFailures = 0;
            _usingTcpFallback = false;
        }
    }

    if (enabled) {
        _udpSender->start();
        _tcpSender->start();
        _startProcessing();
    } else {
        _stopProcessing();
        _udpSender->stop();
        _tcpSender->stop();
        QMutexLocker locker(&_mutex);
        _queue.clear();
    }

    emit enabledChanged();
}

bool LogRemoteSink::isEnabled() const
{
    QMutexLocker locker(&_mutex);
    return _enabled;
}

void LogRemoteSink::setProtocol(Protocol protocol)
{
    {
        QMutexLocker locker(&_mutex);
        if (_protocol == protocol) {
            return;
        }
        _protocol = protocol;
        _consecutiveUdpFailures = 0;
        _usingTcpFallback = false;
    }

    _updateSenderProtocol();

    qCDebug(LogRemoteSinkLog) << "Protocol set to:" << protocol;
    emit protocolChanged();
}

LogRemoteSink::Protocol LogRemoteSink::protocol() const
{
    QMutexLocker locker(&_mutex);
    return _protocol;
}

bool LogRemoteSink::isTcpConnected() const
{
    return _tcpSender->isConnected();
}

void LogRemoteSink::setMaxPendingEntries(int count)
{
    QMutexLocker locker(&_mutex);
    _maxPendingEntries = qMax(1, count);
}

int LogRemoteSink::maxPendingEntries() const
{
    QMutexLocker locker(&_mutex);
    return _maxPendingEntries;
}

void LogRemoteSink::setBatchSize(int size)
{
    QMutexLocker locker(&_mutex);
    _batchSize = qMax(1, size);
}

int LogRemoteSink::batchSize() const
{
    QMutexLocker locker(&_mutex);
    return _batchSize;
}

void LogRemoteSink::setMaxDatagramSize(int bytes)
{
    QMutexLocker locker(&_mutex);
    _maxDatagramSize = qMax(512, bytes);
}

int LogRemoteSink::maxDatagramSize() const
{
    QMutexLocker locker(&_mutex);
    return _maxDatagramSize;
}

void LogRemoteSink::setUdpFailureThreshold(int count)
{
    QMutexLocker locker(&_mutex);
    _udpFailureThreshold = qMax(1, count);
}

int LogRemoteSink::udpFailureThreshold() const
{
    QMutexLocker locker(&_mutex);
    return _udpFailureThreshold;
}

void LogRemoteSink::setTcpConnectTimeout(int ms)
{
    _tcpSender->setConnectTimeout(ms);
}

int LogRemoteSink::tcpConnectTimeout() const
{
    return _tcpSender->connectTimeout();
}

void LogRemoteSink::setTcpReconnectInterval(int ms)
{
    _tcpSender->setReconnectInterval(ms);
}

int LogRemoteSink::tcpReconnectInterval() const
{
    return _tcpSender->reconnectInterval();
}

void LogRemoteSink::setTlsEnabled(bool enabled)
{
    {
        QMutexLocker locker(&_mutex);
        if (_tlsEnabled == enabled) {
            return;
        }
        _tlsEnabled = enabled;
    }

    _tcpSender->setProtocol(enabled ? QGCNetworkSender::Protocol::TLS : QGCNetworkSender::Protocol::TCP);

    qCDebug(LogRemoteSinkLog) << "TLS" << (enabled ? "enabled" : "disabled");
    emit tlsEnabledChanged();
}

bool LogRemoteSink::isTlsEnabled() const
{
    QMutexLocker locker(&_mutex);
    return _tlsEnabled;
}

void LogRemoteSink::setTlsVerifyPeer(bool verify)
{
    {
        QMutexLocker locker(&_mutex);
        if (_tlsVerifyPeer == verify) {
            return;
        }
        _tlsVerifyPeer = verify;
    }

    _tcpSender->setTlsVerifyPeer(verify);

    qCDebug(LogRemoteSinkLog) << "TLS peer verification" << (verify ? "enabled" : "disabled");
    emit tlsVerifyPeerChanged();
}

bool LogRemoteSink::tlsVerifyPeer() const
{
    QMutexLocker locker(&_mutex);
    return _tlsVerifyPeer;
}

void LogRemoteSink::setTlsCaCertificates(const QList<QSslCertificate>& certs)
{
    _tcpSender->setTlsCaCertificates(certs);
}

void LogRemoteSink::setTlsClientCertificate(const QSslCertificate& cert, const QSslKey& key)
{
    _tcpSender->setTlsClientCertificate(cert, key);
}

bool LogRemoteSink::loadTlsCaCertificates(const QString& filePath)
{
    return _tcpSender->loadTlsCaCertificates(filePath);
}

bool LogRemoteSink::loadTlsClientCertificate(const QString& certPath, const QString& keyPath)
{
    return _tcpSender->loadTlsClientCertificate(certPath, keyPath);
}

void LogRemoteSink::setCompressionEnabled(bool enabled)
{
    {
        QMutexLocker locker(&_mutex);
        if (_compressionEnabled == enabled) {
            return;
        }
        _compressionEnabled = enabled;
    }

    qCDebug(LogRemoteSinkLog) << "Compression" << (enabled ? "enabled" : "disabled");
    emit compressionEnabledChanged();
}

bool LogRemoteSink::isCompressionEnabled() const
{
    QMutexLocker locker(&_mutex);
    return _compressionEnabled;
}

void LogRemoteSink::setCompressionLevel(int level)
{
    level = qBound(1, level, 9);

    {
        QMutexLocker locker(&_mutex);
        if (_compressionLevel == level) {
            return;
        }
        _compressionLevel = level;
    }

    qCDebug(LogRemoteSinkLog) << "Compression level set to:" << level;
    emit compressionLevelChanged();
}

int LogRemoteSink::compressionLevel() const
{
    QMutexLocker locker(&_mutex);
    return _compressionLevel;
}

void LogRemoteSink::setMinCompressSize(int bytes)
{
    QMutexLocker locker(&_mutex);
    _minCompressSize = qMax(0, bytes);
}

int LogRemoteSink::minCompressSize() const
{
    QMutexLocker locker(&_mutex);
    return _minCompressSize;
}

void LogRemoteSink::send(const LogEntry& entry)
{
    {
        QMutexLocker locker(&_mutex);
        if (!_enabled) {
            return;
        }

        _queue.enqueue(entry);

        while (_queue.size() > _maxPendingEntries) {
            _queue.dequeue();
        }
    }

    _cv.wakeOne();
}

void LogRemoteSink::_startProcessing()
{
    {
        QMutexLocker locker(&_mutex);
        if (_running) {
            return;
        }
        _running = true;
    }

    _future = QtConcurrent::run(&LogRemoteSink::_processingLoop, this);
    qCDebug(LogRemoteSinkLog) << "Processing started via QtConcurrent";
}

void LogRemoteSink::_stopProcessing()
{
    {
        QMutexLocker locker(&_mutex);
        if (!_running) {
            return;
        }
        _running = false;
    }

    _cv.wakeOne();

    if (_future.isValid()) {
        _future.waitForFinished();
    }

    qCDebug(LogRemoteSinkLog) << "Processing stopped";
}

void LogRemoteSink::_processingLoop()
{
    QQueue<LogEntry> batch;

    while (true) {
        // Wait for entries
        {
            QMutexLocker locker(&_mutex);

            while (_queue.isEmpty() && _running) {
                _cv.wait(&_mutex, kProcessTimeoutMs);
            }

            if (!_running && _queue.isEmpty()) {
                break;
            }

            batch.swap(_queue);
        }

        if (batch.isEmpty()) {
            continue;
        }

        // Get configuration snapshot
        Protocol currentProtocol;
        bool useTcpFallback;
        bool sinkEnabled;
        int maxBatchSize;
        int maxBytes;
        bool compressionEnabled;
        int compressionLevel;
        int minCompressSize;
        {
            QMutexLocker locker(&_mutex);
            currentProtocol = _protocol;
            useTcpFallback = _usingTcpFallback;
            sinkEnabled = _enabled;
            maxBatchSize = _batchSize;
            maxBytes = (currentProtocol == TCP || useTcpFallback)
                ? (_maxDatagramSize * 4)
                : _maxDatagramSize;
            compressionEnabled = _compressionEnabled;
            compressionLevel = _compressionLevel;
            minCompressSize = _minCompressSize;
        }

        if (!sinkEnabled) {
            batch.clear();
            continue;
        }

        // Determine which sender to use
        const bool useTcp = (currentProtocol == TCP) ||
                            (currentProtocol == AutoFallback && useTcpFallback);

        QGCNetworkSender* sender = useTcp ? _tcpSender : _udpSender;

        // Process entries in batches
        int idx = 0;
        while (idx < batch.size()) {
            {
                QMutexLocker locker(&_mutex);
                if (!_running || !_enabled) {
                    batch.clear();
                    break;
                }
            }

            int count = qMin(maxBatchSize, batch.size() - idx);

            // Format batch
            QByteArray data = _formatBatch(batch, idx, count);

            // Check size limit and reduce batch if needed
            while (data.size() > maxBytes && count > 1) {
                count /= 2;
                data = _formatBatch(batch, idx, count);
            }

            if (count == 1 && data.size() > maxBytes && !useTcp) {
                qCWarning(LogRemoteSinkLog) << "Single log entry exceeds max datagram size ("
                                            << data.size() << ">" << maxBytes
                                            << "), UDP delivery may fail";
            }

            // Apply compression
            const LogCompression::Level level = compressionEnabled
                ? static_cast<LogCompression::Level>(compressionLevel)
                : LogCompression::Level::None;
            data = LogCompression::compress(data, level, minCompressSize);

            // Send
            sender->send(data);

            // Check for UDP failure (for auto-fallback).
            // Note: lastSendSucceeded() reflects the result of the most recent send() call
            // within QGCNetworkSender, which queues data asynchronously. The check here is
            // best-effort; the actual send may not have completed yet.
            if (!useTcp && currentProtocol == AutoFallback && !sender->lastSendSucceeded()) {
                _handleUdpFailure();
            }

            idx += count;
        }

        batch.clear();
    }
}

QByteArray LogRemoteSink::_formatBatch(const QQueue<LogEntry>& entries, int startIdx, int count) const
{
    QString vehicleId;
    {
        QMutexLocker locker(&_mutex);
        vehicleId = _vehicleId;
    }

    if (count == 1) {
        // Single entry: send without wrapper
        QJsonObject json = _entryToJson(entries.at(startIdx));
        if (!vehicleId.isEmpty()) {
            json["vid"] = vehicleId;
        }
        return QJsonDocument(json).toJson(QJsonDocument::Compact);
    }

    // Multiple entries: send as batch
    QJsonObject wrapper;
    if (!vehicleId.isEmpty()) {
        wrapper["vid"] = vehicleId;
    }

    QJsonArray logsArray;
    for (int i = 0; i < count; ++i) {
        logsArray.append(_entryToJson(entries.at(startIdx + i)));
    }
    wrapper["logs"] = logsArray;

    return QJsonDocument(wrapper).toJson(QJsonDocument::Compact);
}

QJsonObject LogRemoteSink::_entryToJson(const LogEntry& entry) const
{
    QJsonObject json;
    json["ts"] = entry.timestamp.toString(Qt::ISODateWithMs);
    json["lvl"] = LogEntry::levelLabel(entry.level);
    json["cat"] = entry.category;
    json["msg"] = entry.message;

    if (!entry.function.isEmpty()) {
        if (!entry.file.isEmpty()) {
            json["file"] = entry.file;
        }
        json["fn"] = entry.function;
        json["ln"] = entry.line;
    }

    return json;
}

void LogRemoteSink::_updateSenderProtocol()
{
    bool useTls;
    {
        QMutexLocker locker(&_mutex);
        useTls = _tlsEnabled;
    }
    _tcpSender->setProtocol(useTls ? QGCNetworkSender::Protocol::TLS : QGCNetworkSender::Protocol::TCP);
}

void LogRemoteSink::_handleUdpFailure()
{
    bool switchToTcp = false;
    int failures = 0;
    {
        QMutexLocker locker(&_mutex);
        if (_protocol != AutoFallback) {
            return;
        }
        ++_consecutiveUdpFailures;
        failures = _consecutiveUdpFailures;
        if (failures >= _udpFailureThreshold && !_usingTcpFallback) {
            _usingTcpFallback = true;
            switchToTcp = true;
        }
    }

    if (switchToTcp) {
        qCDebug(LogRemoteSinkLog) << "UDP failure threshold reached (" << failures
                                  << "), switching to TCP fallback";
    }
}

void LogRemoteSink::_onSenderConnected()
{
    emit tcpConnectedChanged();
}

void LogRemoteSink::_onSenderDisconnected()
{
    emit tcpConnectedChanged();
}

void LogRemoteSink::_onDataSent(qint64 bytes)
{
    // Only reset UDP failure counter on successful UDP send
    if (sender() == _udpSender) {
        QMutexLocker locker(&_mutex);
        _consecutiveUdpFailures = 0;
    }
    emit dataSent(bytes);
}
