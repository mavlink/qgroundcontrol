#include "LogManager.h"
#include "LogModel.h"
#include "LogRemoteSink.h"
#include "QGCFileHelper.h"
#include "QGCFileWriter.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QGlobalStatic>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSettings>
#include <QtCore/QThreadPool>

#include <cstring>

Q_STATIC_LOGGING_CATEGORY(LogManagerLog, "Utilities.Logging.LogManager", QtWarningMsg)

// Verify enum mappings stay in sync
static_assert(static_cast<int>(LogManager::RemoteProtocol::UDP) == LogRemoteSink::UDP);
static_assert(static_cast<int>(LogManager::RemoteProtocol::TCP) == LogRemoteSink::TCP);
static_assert(static_cast<int>(LogManager::RemoteProtocol::AutoFallback) == LogRemoteSink::AutoFallback);
static_assert(static_cast<int>(LogManager::ExportFormat::ExportText) == LogFormatter::PlainText);
static_assert(static_cast<int>(LogManager::ExportFormat::ExportJSON) == LogFormatter::JSON);
static_assert(static_cast<int>(LogManager::ExportFormat::ExportCSV) == LogFormatter::CSV);
static_assert(static_cast<int>(LogManager::ExportFormat::ExportJSONLines) == LogFormatter::JSONLines);

Q_GLOBAL_STATIC(LogManager, s_logManager)

static QtMessageHandler s_previousHandler = nullptr;

static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    // Prevent infinite recursion if downstream code (model, file writer, remote sink) logs
    thread_local bool inHandler = false;
    if (inHandler) {
        if (s_previousHandler) {
            s_previousHandler(type, context, msg);
        }
        return;
    }
    inHandler = true;

    // Call previous handler first. QTest::ignoreMessage relies on this ordering.
    if (s_previousHandler) {
        s_previousHandler(type, context, msg);
    }

    // Skip Qt Quick internals (avoid QString allocation on hot path)
    if (context.category && strncmp(context.category, "qt.quick", 8) == 0) {
        inHandler = false;
        return;
    }

    // Build structured entry
    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.level = LogEntry::fromQtMsgType(type);
    entry.category = context.category ? QString(context.category) : QString();
    entry.message = msg;
    entry.file = context.file ? QString(context.file) : QString();
    entry.function = context.function ? QString(context.function) : QString();
    entry.line = context.line;

    // Route to logging system
    if (LogManager::instance()) {
        LogManager::instance()->handleLogEntry(entry);
    }

    inHandler = false;
}

LogManager* LogManager::instance()
{
    return s_logManager();
}

LogManager::LogManager(QObject* parent)
    : QObject(parent)
    , _model(new LogModel(this))
    , _fileWriter(new QGCFileWriter(this))
    , _remoteSink(new LogRemoteSink(this))
{
    connect(_fileWriter, &QGCFileWriter::errorOccurred, this, [this](const QString& msg) {
        _lastError = msg;
        emit lastErrorChanged();
        emit errorOccurred(msg);
        emit errorChanged();
    });

    connect(_fileWriter, &QGCFileWriter::errorCleared, this, [this]() {
        _lastError.clear();
        emit lastErrorChanged();
        emit errorChanged();
    });

    connect(_remoteSink, &LogRemoteSink::errorOccurred, this, [this](const QString& msg) {
        _lastRemoteError = msg;
        emit lastRemoteErrorChanged();
        emit errorOccurred(msg);
    });
    connect(_remoteSink, &LogRemoteSink::tlsErrorOccurred, this, [this](const QString& msg) {
        _lastTlsError = msg;
        emit lastTlsErrorChanged();
        emit tlsErrorOccurred(msg);
    });
    connect(_remoteSink, &LogRemoteSink::dataSent, this, [this](qint64 bytes) {
        _remoteBytesSent += bytes;
        emit remoteBytesSentChanged();
    });
    connect(_remoteSink, &LogRemoteSink::enabledChanged, this, &LogManager::remoteLoggingEnabledChanged);
    connect(_remoteSink, &LogRemoteSink::endpointChanged, this, &LogManager::remoteEndpointChanged);
    connect(_remoteSink, &LogRemoteSink::vehicleIdChanged, this, &LogManager::remoteVehicleIdChanged);
    connect(_remoteSink, &LogRemoteSink::protocolChanged, this, &LogManager::remoteProtocolChanged);
    connect(_remoteSink, &LogRemoteSink::tcpConnectedChanged, this, &LogManager::remoteTcpConnectedChanged);
    connect(_remoteSink, &LogRemoteSink::tlsEnabledChanged, this, &LogManager::remoteTlsEnabledChanged);
    connect(_remoteSink, &LogRemoteSink::tlsVerifyPeerChanged, this, &LogManager::remoteTlsVerifyPeerChanged);
    connect(_remoteSink, &LogRemoteSink::compressionEnabledChanged, this, &LogManager::remoteCompressionEnabledChanged);
    connect(_remoteSink, &LogRemoteSink::compressionLevelChanged, this, &LogManager::remoteCompressionLevelChanged);
    connect(_model, &LogModel::maxEntriesChanged, this, &LogManager::_scheduleSave);

    _fileWriter->setPreOpenCallback([this](const QString& path) {
        return _prepareLogFile(path);
    });
    _fileWriter->setFileSizeCallback([this](qint64 size) {
        if (size >= kMaxLogFileSize) {
            _fileWriter->close();
        }
    });

    _saveTimer = new QTimer(this);
    _saveTimer->setSingleShot(true);
    _saveTimer->setInterval(500);
    connect(_saveTimer, &QTimer::timeout, this, &LogManager::_persistSettings);

    _loadSettings();

    qCDebug(LogManagerLog) << "LogManager initialized";
}

LogManager::~LogManager()
{
    if (_saveTimer->isActive()) {
        _saveTimer->stop();
        _persistSettings();
    }
    flush();

    qCDebug(LogManagerLog) << "LogManager destroyed";
}

void LogManager::installHandler()
{
    static bool installed = false;
    if (installed) {
        return;
    }
    installed = true;

    qSetMessagePattern(QStringLiteral("%{time process} %{if-warning}W:%{endif}%{if-critical}C:%{endif}%{if-fatal}F:%{endif} %{message} - %{category} (%{function}:%{line})"));
    s_previousHandler = qInstallMessageHandler(messageHandler);
}

void LogManager::handleLogEntry(const LogEntry& entry)
{
    _model->append(entry);

    if (_diskLoggingEnabled.load(std::memory_order_relaxed)) {
        const QString formatted = LogFormatter::formatText(entry);
        _fileWriter->write(formatted + QStringLiteral("\n"));
    }

    _remoteSink->send(entry);
}

// ----------------------------------------------------------------------------
// Disk logging
// ----------------------------------------------------------------------------

bool LogManager::isDiskLoggingEnabled() const
{
    return _diskLoggingEnabled.load(std::memory_order_relaxed);
}

void LogManager::setDiskLoggingEnabled(bool enabled)
{
    if (_diskLoggingEnabled.load(std::memory_order_relaxed) == enabled) {
        return;
    }

    _diskLoggingEnabled.store(enabled, std::memory_order_release);
    _applyDiskWriterState();

    _scheduleSave();
    emit diskLoggingEnabledChanged();

    qCDebug(LogManagerLog) << "Disk logging" << (enabled ? "enabled" : "disabled");
}

void LogManager::setLogDirectory(const QString& path)
{
    if (path.isEmpty()) {
        _fileWriter->setFilePath(QString());
    } else {
        const QDir dir(path);
        const QString filePath = dir.absoluteFilePath("QGCConsole.log");
        _fileWriter->setFilePath(filePath);
    }
    _applyDiskWriterState();

    qCDebug(LogManagerLog) << "Log directory set to:" << path;
}

// ----------------------------------------------------------------------------
// Remote logging
// ----------------------------------------------------------------------------

bool LogManager::isRemoteLoggingEnabled() const
{
    return _remoteSink->isEnabled();
}

void LogManager::setRemoteLoggingEnabled(bool enabled)
{
    if (_remoteSink->isEnabled() == enabled) {
        return;
    }
    _remoteSink->setEnabled(enabled);
    _scheduleSave();
    qCDebug(LogManagerLog) << "Remote logging" << (enabled ? "enabled" : "disabled");
}

QString LogManager::remoteEndpoint() const
{
    return _remoteSink->endpoint();
}

void LogManager::setRemoteEndpoint(const QString& endpoint)
{
    const QString trimmed = endpoint.trimmed();
    if (!trimmed.isEmpty()) {
        const int lastColon = trimmed.lastIndexOf(':');
        if (lastColon <= 0 || lastColon == trimmed.size() - 1) {
            qCWarning(LogManagerLog) << "Invalid endpoint format (expected host:port):" << trimmed;
            return;
        }
        bool portOk = false;
        const int port = trimmed.mid(lastColon + 1).toInt(&portOk);
        if (!portOk || port <= 0 || port > 65535) {
            qCWarning(LogManagerLog) << "Invalid port in endpoint:" << trimmed;
            return;
        }
    }

    _remoteSink->setEndpoint(trimmed);
    _scheduleSave();
    qCDebug(LogManagerLog) << "Remote endpoint set to:" << trimmed;
}

QString LogManager::remoteVehicleId() const
{
    return _remoteSink->vehicleId();
}

void LogManager::setRemoteVehicleId(const QString& id)
{
    _remoteSink->setVehicleId(id);
    _scheduleSave();
    qCDebug(LogManagerLog) << "Remote vehicle ID set to:" << id;
}

LogManager::RemoteProtocol LogManager::remoteProtocol() const
{
    return static_cast<RemoteProtocol>(_remoteSink->protocol());
}

void LogManager::setRemoteProtocol(RemoteProtocol protocol)
{
    if (remoteProtocol() == protocol) {
        return;
    }
    _remoteSink->setProtocol(static_cast<LogRemoteSink::Protocol>(protocol));
    _scheduleSave();
    qCDebug(LogManagerLog) << "Remote protocol set to:" << protocol;
}

bool LogManager::isRemoteTcpConnected() const
{
    return _remoteSink->isTcpConnected();
}

bool LogManager::isRemoteTlsEnabled() const
{
    return _remoteSink->isTlsEnabled();
}

void LogManager::setRemoteTlsEnabled(bool enabled)
{
    if (_remoteSink->isTlsEnabled() == enabled) {
        return;
    }
    _remoteSink->setTlsEnabled(enabled);
    _scheduleSave();
    qCDebug(LogManagerLog) << "Remote TLS" << (enabled ? "enabled" : "disabled");
}

bool LogManager::remoteTlsVerifyPeer() const
{
    return _remoteSink->tlsVerifyPeer();
}

void LogManager::setRemoteTlsVerifyPeer(bool verify)
{
    if (_remoteSink->tlsVerifyPeer() == verify) {
        return;
    }
    _remoteSink->setTlsVerifyPeer(verify);
    _scheduleSave();
    qCDebug(LogManagerLog) << "Remote TLS verify peer:" << verify;
}

bool LogManager::isRemoteCompressionEnabled() const
{
    return _remoteSink->isCompressionEnabled();
}

void LogManager::setRemoteCompressionEnabled(bool enabled)
{
    if (_remoteSink->isCompressionEnabled() == enabled) {
        return;
    }
    _remoteSink->setCompressionEnabled(enabled);
    _scheduleSave();
    qCDebug(LogManagerLog) << "Remote compression" << (enabled ? "enabled" : "disabled");
}

int LogManager::remoteCompressionLevel() const
{
    return _remoteSink->compressionLevel();
}

void LogManager::setRemoteCompressionLevel(int level)
{
    if (_remoteSink->compressionLevel() == level) {
        return;
    }
    _remoteSink->setCompressionLevel(level);
    _scheduleSave();
    qCDebug(LogManagerLog) << "Remote compression level set to:" << level;
}

// ----------------------------------------------------------------------------
// Utility
// ----------------------------------------------------------------------------

QString LogManager::formatHex(const QByteArray& data, int maxBytes)
{
    if (data.isEmpty()) {
        return QString();
    }

    const int bytesToFormat = (maxBytes > 0 && maxBytes < data.size()) ? maxBytes : data.size();

    QString result = data.left(bytesToFormat).toHex(' ').toUpper();
    if (maxBytes > 0 && data.size() > maxBytes) {
        result += QStringLiteral("... (%1 more bytes)").arg(data.size() - maxBytes);
    }
    return result;
}

bool LogManager::hasError() const
{
    return _fileWriter->hasError();
}

void LogManager::clearError()
{
    _lastError.clear();
    _lastRemoteError.clear();
    _lastTlsError.clear();
    _fileWriter->clearError();
    emit lastErrorChanged();
    emit lastRemoteErrorChanged();
    emit lastTlsErrorChanged();
}

void LogManager::clear()
{
    _model->clear();
    qCDebug(LogManagerLog) << "Logs cleared";
}

void LogManager::flush()
{
    _fileWriter->flush();
}

void LogManager::exportToFile(const QString& destFile, ExportFormat format)
{
    _exportEntries(_model->allEntries(), destFile, format);
}

void LogManager::exportFilteredToFile(const QString& destFile, ExportFormat format)
{
    _exportEntries(_model->filteredEntries(), destFile, format);
}

void LogManager::_exportEntries(const QList<LogEntry>& entries, const QString& destFile, ExportFormat format)
{
    emit exportStarted();

    QPointer<LogManager> guard(this);
    QThreadPool::globalInstance()->start([guard, destFile, entries, format]() {
        const QString content = LogFormatter::format(entries, static_cast<LogFormatter::Format>(format));
        const bool success = QGCFileHelper::atomicWrite(destFile, content.toUtf8());
        if (!success) {
            qCWarning(LogManagerLog) << "Export failed:" << destFile;
        }

        // Post result back to main thread. QueuedConnection is safe even if target is destroyed.
        QMetaObject::invokeMethod(qApp, [guard, success]() {
            if (guard) {
                emit guard->exportFinished(success);
            }
        }, Qt::QueuedConnection);
    });
}

void LogManager::resetRemoteBytesSent()
{
    _remoteBytesSent = 0;
    emit remoteBytesSentChanged();
}

bool LogManager::loadRemoteTlsCaCertificates(const QString& filePath)
{
    return _remoteSink->loadTlsCaCertificates(filePath);
}

bool LogManager::loadRemoteTlsClientCertificate(const QString& certPath, const QString& keyPath)
{
    return _remoteSink->loadTlsClientCertificate(certPath, keyPath);
}

QString LogManager::exportFileFilter()
{
    return QStringLiteral("Text files (*.txt);;JSON files (*.json);;CSV files (*.csv);;JSON Lines (*.jsonl);;All files (*)");
}

// ----------------------------------------------------------------------------
// Settings persistence
// ----------------------------------------------------------------------------

void LogManager::_loadSettings()
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);

    // Disk logging
    const bool diskEnabled = settings.value("diskLoggingEnabled", false).toBool();
    _diskLoggingEnabled.store(diskEnabled, std::memory_order_release);

    // Remote logging - basic
    const QString endpoint = settings.value("remoteEndpoint").toString();
    if (!endpoint.isEmpty()) {
        _remoteSink->setEndpoint(endpoint);
    }

    const QString vehicleId = settings.value("remoteVehicleId").toString();
    if (!vehicleId.isEmpty()) {
        _remoteSink->setVehicleId(vehicleId);
    }

    // Remote logging - protocol
    const int protocol = settings.value("remoteProtocol", static_cast<int>(RemoteProtocol::UDP)).toInt();
    if (protocol >= 0 && protocol <= static_cast<int>(RemoteProtocol::AutoFallback)) {
        _remoteSink->setProtocol(static_cast<LogRemoteSink::Protocol>(protocol));
    }

    // Remote logging - TLS
    const bool tlsEnabled = settings.value("remoteTlsEnabled", false).toBool();
    _remoteSink->setTlsEnabled(tlsEnabled);

    const bool tlsVerifyPeer = settings.value("remoteTlsVerifyPeer", true).toBool();
    _remoteSink->setTlsVerifyPeer(tlsVerifyPeer);

    // Remote logging - compression
    const bool compressionEnabled = settings.value("remoteCompressionEnabled", false).toBool();
    _remoteSink->setCompressionEnabled(compressionEnabled);

    const int compressionLevel = qBound(1, settings.value("remoteCompressionLevel", 6).toInt(), 9);
    _remoteSink->setCompressionLevel(compressionLevel);

    // Buffer size
    const int maxEntries = settings.value("maxEntries", 100000).toInt();
    _model->setMaxEntries(qBound(10000, maxEntries, 500000));

    // Remote logging - enabled (load last to apply after configuration)
    const bool remoteEnabled = settings.value("remoteLoggingEnabled", false).toBool();
    _remoteSink->setEnabled(remoteEnabled);

    qCDebug(LogManagerLog) << "Settings loaded";
}

void LogManager::_scheduleSave()
{
    _saveTimer->start();
}

void LogManager::_persistSettings()
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);

    settings.setValue("diskLoggingEnabled", _diskLoggingEnabled.load(std::memory_order_relaxed));
    settings.setValue("remoteLoggingEnabled", _remoteSink->isEnabled());
    settings.setValue("remoteEndpoint", _remoteSink->endpoint());
    settings.setValue("remoteVehicleId", _remoteSink->vehicleId());
    settings.setValue("remoteProtocol", static_cast<int>(_remoteSink->protocol()));
    settings.setValue("remoteTlsEnabled", _remoteSink->isTlsEnabled());
    settings.setValue("remoteTlsVerifyPeer", _remoteSink->tlsVerifyPeer());
    settings.setValue("remoteCompressionEnabled", _remoteSink->isCompressionEnabled());
    settings.setValue("remoteCompressionLevel", _remoteSink->compressionLevel());
    settings.setValue("maxEntries", _model->maxEntries());

    qCDebug(LogManagerLog) << "Settings saved";
}

bool LogManager::_prepareLogFile(const QString& path) const
{
    const QFileInfo info(path);
    if (!info.exists() || info.size() < kMaxLogFileSize) {
        return true;
    }

    _rotateLogFiles(path);
    return true;
}

void LogManager::_rotateLogFiles(const QString& path) const
{
    const QFileInfo fileInfo(path);
    const QString dir = fileInfo.absolutePath();
    const QString name = fileInfo.baseName();
    const QString ext = fileInfo.completeSuffix();

    for (int i = kMaxBackupFiles - 1; i >= 1; --i) {
        const QString from = QStringLiteral("%1/%2.%3.%4").arg(dir, name).arg(i).arg(ext);
        const QString to = QStringLiteral("%1/%2.%3.%4").arg(dir, name).arg(i + 1).arg(ext);
        if (QFile::exists(to)) {
            (void) QFile::remove(to);
        }
        if (QFile::exists(from)) {
            (void) QFile::rename(from, to);
        }
    }

    const QString firstBackup = QStringLiteral("%1/%2.1.%3").arg(dir, name, ext);
    if (QFile::exists(firstBackup)) {
        (void) QFile::remove(firstBackup);
    }
    (void) QFile::rename(path, firstBackup);
}

void LogManager::_applyDiskWriterState()
{
    const bool enabled = _diskLoggingEnabled.load(std::memory_order_relaxed);
    if (!enabled) {
        _fileWriter->stop();
        return;
    }

    if (_fileWriter->filePath().isEmpty()) {
        _fileWriter->stop();
        qCWarning(LogManagerLog) << "Disk logging enabled but log file path is not configured yet";
        return;
    }

    _fileWriter->start();
}
