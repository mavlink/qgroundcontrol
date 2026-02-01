#include "LogManager.h"
#include "LogDiskWriter.h"
#include "LogEntry.h"
#include "LogModel.h"
#include "LogRemoteSink.h"
#include "QGCFileHelper.h"
#include <QtCore/QLoggingCategory>

#include <QtCore/QDir>
#include <QtCore/QThreadPool>
#include <QtCore/QGlobalStatic>
#include <QtCore/QSettings>

Q_STATIC_LOGGING_CATEGORY(LogManagerLog, "Utilities.Logging.LogManager", QtWarningMsg)

Q_GLOBAL_STATIC(LogManager, s_logManager)

static QtMessageHandler s_previousHandler = nullptr;

static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    // Skip Qt Quick internals
    if (context.category && QString(context.category).startsWith("qt.quick")) {
        if (s_previousHandler) {
            s_previousHandler(type, context, msg);
        }
        return;
    }

    // Build structured entry
    QGCLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.level = QGCLogEntry::fromQtMsgType(type);
    entry.category = context.category ? QString(context.category) : QString();
    entry.message = msg;
    entry.file = context.file ? QString(context.file) : QString();
    entry.function = context.function ? QString(context.function) : QString();
    entry.line = context.line;

    // Route to logging system
    if (LogManager::instance()) {
        LogManager::instance()->handleLogEntry(entry);
    }

    // Chain to previous handler
    if (s_previousHandler) {
        s_previousHandler(type, context, msg);
    }
}

LogManager* LogManager::instance()
{
    return s_logManager();
}

LogManager::LogManager(QObject* parent)
    : QObject(parent)
    , _model(new LogModel(this))
    , _diskWriter(new LogDiskWriter(this))
    , _remoteSink(new LogRemoteSink(this))
{
    connect(_diskWriter, &LogDiskWriter::errorOccurred, this, [this](const QString& msg) {
        emit errorOccurred(msg);
        emit errorChanged();
    });

    connect(_diskWriter, &LogDiskWriter::errorCleared, this, [this]() {
        emit errorChanged();
    });

    connect(_remoteSink, &LogRemoteSink::errorOccurred, this, &LogManager::errorOccurred);
    connect(_remoteSink, &LogRemoteSink::enabledChanged, this, &LogManager::remoteLoggingEnabledChanged);
    connect(_remoteSink, &LogRemoteSink::endpointChanged, this, &LogManager::remoteEndpointChanged);
    connect(_remoteSink, &LogRemoteSink::vehicleIdChanged, this, &LogManager::remoteVehicleIdChanged);
    connect(_remoteSink, &LogRemoteSink::protocolChanged, this, &LogManager::remoteProtocolChanged);
    connect(_remoteSink, &LogRemoteSink::tcpConnectedChanged, this, &LogManager::remoteTcpConnectedChanged);
    connect(_remoteSink, &LogRemoteSink::tlsEnabledChanged, this, &LogManager::remoteTlsEnabledChanged);
    connect(_remoteSink, &LogRemoteSink::tlsVerifyPeerChanged, this, &LogManager::remoteTlsVerifyPeerChanged);
    connect(_remoteSink, &LogRemoteSink::compressionEnabledChanged, this, &LogManager::remoteCompressionEnabledChanged);
    connect(_remoteSink, &LogRemoteSink::compressionLevelChanged, this, &LogManager::remoteCompressionLevelChanged);

    _loadSettings();

    qCDebug(LogManagerLog) << "LogManager initialized";
}

LogManager::~LogManager()
{
    flush();
    qCDebug(LogManagerLog) << "LogManager destroyed";
}

void LogManager::installHandler()
{
    qSetMessagePattern(QStringLiteral("%{time process} %{if-warning}W:%{endif}%{if-critical}C:%{endif}%{if-fatal}F:%{endif} %{message} - %{category} (%{function}:%{line})"));
    s_previousHandler = qInstallMessageHandler(messageHandler);
}

void LogManager::handleLogEntry(const QGCLogEntry& entry)
{
    _model->append(entry);
    _diskWriter->write(entry);
    _remoteSink->send(entry);
}

bool LogManager::isDiskLoggingEnabled() const
{
    return _diskWriter->isEnabled();
}

void LogManager::setDiskLoggingEnabled(bool enabled)
{
    if (_diskWriter->isEnabled() == enabled) {
        return;
    }

    _diskWriter->setEnabled(enabled);
    _saveRemoteSettings();
    emit diskLoggingEnabledChanged();

    qCDebug(LogManagerLog) << "Disk logging" << (enabled ? "enabled" : "disabled");
}

void LogManager::setLogDirectory(const QString& path)
{
    const QDir dir(path);
    const QString filePath = dir.absoluteFilePath("QGCConsole.log");
    _diskWriter->setFilePath(filePath);

    qCDebug(LogManagerLog) << "Log directory set to:" << path;
}

bool LogManager::isRemoteLoggingEnabled() const
{
    return _remoteSink->isEnabled();
}

void LogManager::setRemoteLoggingEnabled(bool enabled)
{
    _remoteSink->setEnabled(enabled);
    _saveRemoteSettings();
    qCDebug(LogManagerLog) << "Remote logging" << (enabled ? "enabled" : "disabled");
}

QString LogManager::remoteEndpoint() const
{
    return _remoteSink->endpoint();
}

void LogManager::setRemoteEndpoint(const QString& endpoint)
{
    _remoteSink->setEndpoint(endpoint);
    _saveRemoteSettings();
    qCDebug(LogManagerLog) << "Remote endpoint set to:" << endpoint;
}

QString LogManager::remoteVehicleId() const
{
    return _remoteSink->vehicleId();
}

void LogManager::setRemoteVehicleId(const QString& id)
{
    _remoteSink->setVehicleId(id);
    _saveRemoteSettings();
    qCDebug(LogManagerLog) << "Remote vehicle ID set to:" << id;
}

LogManager::RemoteProtocol LogManager::remoteProtocol() const
{
    return static_cast<RemoteProtocol>(_remoteSink->protocol());
}

void LogManager::setRemoteProtocol(RemoteProtocol protocol)
{
    _remoteSink->setProtocol(static_cast<LogRemoteSink::Protocol>(protocol));
    _saveRemoteSettings();
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
    _remoteSink->setTlsEnabled(enabled);
    _saveRemoteSettings();
    qCDebug(LogManagerLog) << "Remote TLS" << (enabled ? "enabled" : "disabled");
}

bool LogManager::remoteTlsVerifyPeer() const
{
    return _remoteSink->tlsVerifyPeer();
}

void LogManager::setRemoteTlsVerifyPeer(bool verify)
{
    _remoteSink->setTlsVerifyPeer(verify);
    _saveRemoteSettings();
    qCDebug(LogManagerLog) << "Remote TLS verify peer:" << verify;
}

bool LogManager::isRemoteCompressionEnabled() const
{
    return _remoteSink->isCompressionEnabled();
}

void LogManager::setRemoteCompressionEnabled(bool enabled)
{
    _remoteSink->setCompressionEnabled(enabled);
    _saveRemoteSettings();
    qCDebug(LogManagerLog) << "Remote compression" << (enabled ? "enabled" : "disabled");
}

int LogManager::remoteCompressionLevel() const
{
    return _remoteSink->compressionLevel();
}

void LogManager::setRemoteCompressionLevel(int level)
{
    _remoteSink->setCompressionLevel(level);
    _saveRemoteSettings();
    qCDebug(LogManagerLog) << "Remote compression level set to:" << level;
}

bool LogManager::hasError() const
{
    return _diskWriter->hasError();
}

void LogManager::clearError()
{
    _diskWriter->clearError();
}

void LogManager::clear()
{
    _model->clear();
    qCDebug(LogManagerLog) << "Logs cleared";
}

void LogManager::flush()
{
    _diskWriter->flush();
}

void LogManager::exportToFile(const QString& destFile, ExportFormat format)
{
    const QList<QGCLogEntry> entries = _model->allEntries();

    emit exportStarted();

    QThreadPool::globalInstance()->start([this, destFile, entries, format]() {
        // Format content
        const QString content = LogFormatter::format(entries, static_cast<LogFormatter::Format>(format));

        // Atomic write (uses temp file + rename)
        const bool success = QGCFileHelper::atomicWrite(destFile, content.toUtf8());
        if (!success) {
            qCWarning(LogManagerLog) << "Export failed:" << destFile;
        }

        // Emit on main thread
        QMetaObject::invokeMethod(this, [this, success]() {
            emit exportFinished(success);
        }, Qt::QueuedConnection);
    });
}

void LogManager::exportFilteredToFile(const QString& destFile, ExportFormat format)
{
    const QList<QGCLogEntry> entries = _model->filteredEntries();

    emit exportStarted();

    QThreadPool::globalInstance()->start([this, destFile, entries, format]() {
        // Format content
        const QString content = LogFormatter::format(entries, static_cast<LogFormatter::Format>(format));

        // Atomic write (uses temp file + rename)
        const bool success = QGCFileHelper::atomicWrite(destFile, content.toUtf8());
        if (!success) {
            qCWarning(LogManagerLog) << "Export filtered failed:" << destFile;
        }

        // Emit on main thread
        QMetaObject::invokeMethod(this, [this, success]() {
            emit exportFinished(success);
        }, Qt::QueuedConnection);
    });
}

QString LogManager::exportFileFilter()
{
    return QStringLiteral("Text files (*.txt);;JSON files (*.json);;CSV files (*.csv);;JSON Lines (*.jsonl);;All files (*)");
}

void LogManager::_loadSettings()
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);

    // Disk logging
    const bool diskEnabled = settings.value("diskLoggingEnabled", false).toBool();
    _diskWriter->setEnabled(diskEnabled);

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
    const int protocol = settings.value("remoteProtocol", static_cast<int>(UDP)).toInt();
    _remoteSink->setProtocol(static_cast<LogRemoteSink::Protocol>(protocol));

    // Remote logging - TLS
    const bool tlsEnabled = settings.value("remoteTlsEnabled", false).toBool();
    _remoteSink->setTlsEnabled(tlsEnabled);

    const bool tlsVerifyPeer = settings.value("remoteTlsVerifyPeer", true).toBool();
    _remoteSink->setTlsVerifyPeer(tlsVerifyPeer);

    // Remote logging - compression
    const bool compressionEnabled = settings.value("remoteCompressionEnabled", false).toBool();
    _remoteSink->setCompressionEnabled(compressionEnabled);

    const int compressionLevel = settings.value("remoteCompressionLevel", 6).toInt();
    _remoteSink->setCompressionLevel(compressionLevel);

    // Remote logging - enabled (load last to apply after configuration)
    const bool remoteEnabled = settings.value("remoteLoggingEnabled", false).toBool();
    _remoteSink->setEnabled(remoteEnabled);

    qCDebug(LogManagerLog) << "Settings loaded";
}

void LogManager::_saveRemoteSettings()
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);

    settings.setValue("diskLoggingEnabled", _diskWriter->isEnabled());
    settings.setValue("remoteLoggingEnabled", _remoteSink->isEnabled());
    settings.setValue("remoteEndpoint", _remoteSink->endpoint());
    settings.setValue("remoteVehicleId", _remoteSink->vehicleId());
    settings.setValue("remoteProtocol", static_cast<int>(_remoteSink->protocol()));
    settings.setValue("remoteTlsEnabled", _remoteSink->isTlsEnabled());
    settings.setValue("remoteTlsVerifyPeer", _remoteSink->tlsVerifyPeer());
    settings.setValue("remoteCompressionEnabled", _remoteSink->isCompressionEnabled());
    settings.setValue("remoteCompressionLevel", _remoteSink->compressionLevel());

    qCDebug(LogManagerLog) << "Settings saved";
}
