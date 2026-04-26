#include "LogManager.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QCoreApplication>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QPointer>
#include <QtCore/QSaveFile>
#include <QtCore/QThread>
#include <QtQml/QJSEngine>
#include <atomic>
#include <cstring>

#include "LogFormatter.h"
#include "LogModel.h"
#include "LogRemoteSink.h"
#include "LogStore.h"
#include "LogStoreQueryModel.h"
#include "QGCCompression.h"
#include "QGCFileWriter.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(LogManagerLog, "Utilities.LogManager")

static std::atomic<LogManager*> s_instance{nullptr};

// ---------------------------------------------------------------------------
// Test capture storage
// ---------------------------------------------------------------------------

static std::atomic<bool> s_captureEnabled{false};
static QMutex s_captureMutex;
static QList<LogEntry> s_capturedMessages;

// ---------------------------------------------------------------------------
// Qt message handler
// ---------------------------------------------------------------------------

static QtMessageHandler s_defaultHandler = nullptr;

void LogManager::msgHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    auto* inst = s_instance.load(std::memory_order_acquire);

    if (s_defaultHandler) {
        s_defaultHandler(type, context, msg);
    }

    if (!inst) {
        return;
    }

    // Suppress noisy Qt Quick internals (also matches qt.quickcontrols, etc.)
    if (context.category && std::strncmp(context.category, "qt.quick", 8) == 0) {
        return;
    }

    LogManager::captureIfEnabled(type, context, msg);
    inst->log(type, context, msg);
}

// ---------------------------------------------------------------------------
// LogManager (manager)
// ---------------------------------------------------------------------------

LogManager* LogManager::instance()
{
    return s_instance.load(std::memory_order_acquire);
}

LogManager* LogManager::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(jsEngine);
    auto* inst = instance();
    Q_ASSERT(inst);
    QJSEngine::setObjectOwnership(inst, QJSEngine::CppOwnership);
    return inst;
}

LogManager::LogManager(QObject* parent) : QObject(parent)
{
    s_instance.store(this, std::memory_order_release);
    _model = new LogModel(this);
    _remoteSink = new LogRemoteSink(this);
    _logStore = new LogStore(this);
    _historyModel = new LogStoreQueryModel(_logStore, this);
    _fileWriter = new QGCFileWriter(this);

    (void)connect(_fileWriter, &QGCFileWriter::errorOccurred, this, [this](const QString& msg) { _setIoError(msg); });
    (void)connect(_fileWriter, &QGCFileWriter::fileSizeChanged, this, [this](qint64 size) {
        if (size >= kMaxLogFileSize) {
            _rotateLogs();
        }
    });

    _flushTimer.setInterval(kFlushIntervalMSecs);
    _flushTimer.setSingleShot(false);
    (void)connect(&_flushTimer, &QTimer::timeout, this, &LogManager::_flushToDisk);
    _flushTimer.start();
}

LogManager::~LogManager()
{
    // Detach from the message handler so no new entries are queued.
    if (s_instance.load(std::memory_order_relaxed) == this) {
        s_instance.store(nullptr, std::memory_order_release);
    }

    // Drain any already-queued invokeMethod lambdas that reference us.
    QCoreApplication::processEvents();

    _flushTimer.stop();
    _flushToDisk();
    _remoteSink->setEnabled(false);
    _fileWriter->close();
    _logStore->close();

    if (_exportFuture.isValid()) {
        _exportFuture.waitForFinished();
    }
}

void LogManager::installHandler()
{
    Q_ASSERT(!s_instance.load(std::memory_order_relaxed));
    auto* mgr = new LogManager();

    qSetMessagePattern(
        QStringLiteral("%{time process}%{if-warning} Warning:%{endif}%{if-critical} Critical:%{endif} %{message} - "
                       "%{category} - (%{function}:%{line})"));
    s_defaultHandler = qInstallMessageHandler(&LogManager::msgHandler);
}

void LogManager::applyEnvironmentLogLevel()
{
    const QByteArray env = qgetenv("QGC_LOG_LEVEL");
    if (env.isEmpty()) {
        return;
    }

    const QString level = QString::fromUtf8(env).toLower().trimmed();
    QString rules;

    if (level == QStringLiteral("trace") || level == QStringLiteral("debug")) {
        rules = QStringLiteral("*.debug=true\n");
    } else if (level == QStringLiteral("info")) {
        rules = QStringLiteral("*.debug=false\n*.info=true\n");
    } else if (level == QStringLiteral("warning") || level == QStringLiteral("warn")) {
        rules = QStringLiteral("*.debug=false\n*.info=false\n*.warning=true\n");
    } else if (level == QStringLiteral("critical") || level == QStringLiteral("error")) {
        rules = QStringLiteral("*.debug=false\n*.info=false\n*.warning=false\n*.critical=true\n");
    } else if (level == QStringLiteral("off") || level == QStringLiteral("none")) {
        rules = QStringLiteral("*.debug=false\n*.info=false\n*.warning=false\n*.critical=false\n");
    } else {
        qWarning("QGC_LOG_LEVEL: unknown level '%s' (use debug/info/warning/critical/off)", env.constData());
        return;
    }

    QLoggingCategory::setFilterRules(rules);
}

// ---------------------------------------------------------------------------
// Log ingestion
// ---------------------------------------------------------------------------

void LogManager::log(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    LogEntry entry = buildEntry(type, context, message);

    QMetaObject::invokeMethod(
        this,
        [this, entry = std::move(entry)]() mutable {
            entry.category = _internCategory(entry.category);
            _handleEntry(entry);
        },
        Qt::QueuedConnection);
}

const QString& LogManager::_internCategory(const QString& category)
{
    auto it = _internedCategories.find(category);
    if (it != _internedCategories.end()) {
        return *it;
    }
    return *_internedCategories.insert(category);
}

void LogManager::_dispatchToSinks(const LogEntry& entry)
{
    _model->enqueue(entry);
    if (_logStore->isOpen()) {
        _logStore->append(entry);
    }
    if (_diskLoggingEnabled) {
        _pendingDiskWrites.append(entry);
    }
    if (_remoteSink) {
        _remoteSink->send(entry);
    }
}

void LogManager::_handleEntry(const LogEntry& entry)
{
    if (!_rateLimitCheck(entry)) {
        return;
    }

    _dispatchToSinks(entry);

    if (_flushOnLevel >= 0 && static_cast<int>(entry.level) >= _flushOnLevel) {
        _flushToDisk();
    }
}

bool LogManager::_rateLimitCheck(const LogEntry& entry)
{
    if (entry.category.isEmpty()) {
        return true;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    auto& bucket = _rateBuckets[entry.category];

    if (bucket.lastRefillMs == 0) {
        bucket.lastRefillMs = now;
        bucket.tokens = kRateMaxTokens;
    }

    const qint64 elapsed = now - bucket.lastRefillMs;
    if (elapsed > 0) {
        const int refill = static_cast<int>(elapsed * kRateTokensPerSecond / 1000);
        if (refill > 0) {
            bucket.tokens = qMin(bucket.tokens + refill, kRateMaxTokens);
            bucket.lastRefillMs = now;

            if (bucket.suppressed > 0 && bucket.tokens > 0) {
                _emitSuppressedSummary(entry.category, bucket.suppressed);
                bucket.suppressed = 0;
            }
        }
    }

    if (bucket.tokens > 0) {
        --bucket.tokens;
        return true;
    }

    ++bucket.suppressed;
    return false;
}

void LogManager::_emitSuppressedSummary(const QString& category, int count)
{
    LogEntry summary;
    summary.timestamp = QDateTime::currentDateTime();
    summary.level = LogEntry::Warning;
    summary.category = category;
    summary.message = QStringLiteral("... %1 messages suppressed (rate limited)").arg(count);
    summary.buildFormatted();

    _dispatchToSinks(summary);
}

// ---------------------------------------------------------------------------
// Manager operations
// ---------------------------------------------------------------------------

void LogManager::clearError()
{
    if (_ioError) {
        _ioError = false;
        _lastError.clear();
        emit hasErrorChanged();
        emit lastErrorChanged();
    }
}

void LogManager::flush()
{
    Q_ASSERT(QThread::currentThread() == thread());
    _flushToDisk();
    _fileWriter->flush();
}

void LogManager::setDiskLoggingEnabled(bool enabled)
{
    if (_diskLoggingEnabled != enabled) {
        if (!enabled) {
            _flushToDisk();
            _fileWriter->close();
        }
        _diskLoggingEnabled = enabled;
        emit diskLoggingEnabledChanged();
    }
}

void LogManager::setDiskCompressionEnabled(bool enabled)
{
    if (_diskCompressionEnabled != enabled) {
        _diskCompressionEnabled = enabled;
        emit diskCompressionEnabledChanged();
    }
}

void LogManager::setFlushOnLevel(int level)
{
    if (_flushOnLevel != level) {
        _flushOnLevel = level;
        emit flushOnLevelChanged();
    }
}

QStringList LogManager::categoryLogLevelNames()
{
    return {tr("Debug"), tr("Info"), tr("Warning"), tr("Critical")};
}

QVariantList LogManager::categoryLogLevelValues()
{
    return {QtDebugMsg, QtInfoMsg, QtWarningMsg, QtCriticalMsg};
}

// ---------------------------------------------------------------------------
// Disk writing
// ---------------------------------------------------------------------------

void LogManager::_setIoError(const QString& message)
{
    _ioError = true;
    _lastError = message;
    emit hasErrorChanged();
    emit lastErrorChanged();
}

void LogManager::setLogDirectory(const QString& path)
{
    if (_logDirectory == path) {
        return;
    }
    _logDirectory = path;

    if (_logStore->isOpen()) {
        _logStore->close();
    }

    if (path.isEmpty()) {
        _fileWriter->setFilePath(QString());
        return;
    }

    const QDir dir(path);
    _fileWriter->setFilePath(dir.absoluteFilePath(QStringLiteral("QGCConsole.log")));
    _logStore->open(dir.absoluteFilePath(QStringLiteral("QGCConsole.db")));
}

void LogManager::_rotateLogs()
{
    _fileWriter->flush();
    _fileWriter->close();

    const QString path = _fileWriter->filePath();
    const QFileInfo fileInfo(path);
    const QString dir = fileInfo.absolutePath();
    const QString name = fileInfo.baseName();
    const QString ext = fileInfo.completeSuffix();

    for (int i = kMaxBackupFiles - 1; i >= 1; --i) {
        const QString from = QStringLiteral("%1/%2.%3.%4").arg(dir, name).arg(i).arg(ext);
        const QString to = QStringLiteral("%1/%2.%3.%4").arg(dir, name).arg(i + 1).arg(ext);
        if (QFile::exists(to)) {
            (void)QFile::remove(to);
        }
        if (QFile::exists(from)) {
            (void)QFile::rename(from, to);
        }
    }

    const QString firstBackup = QStringLiteral("%1/%2.1.%3").arg(dir, name, ext);
    (void)QFile::rename(path, firstBackup);

    _fileWriter->setFilePath(path);
}

void LogManager::_flushToDisk()
{
    if (_pendingDiskWrites.isEmpty() || _ioError || !_diskLoggingEnabled || _logDirectory.isEmpty()) {
        return;
    }

    auto entries = std::move(_pendingDiskWrites);
    _fileWriter->write(LogFormatter::formatAsText(entries));
}

// ---------------------------------------------------------------------------
// Export
// ---------------------------------------------------------------------------

void LogManager::writeMessages(const QString& destFile, ExportFormat format)
{
    _exportEntries(_model->allEntriesSnapshot(), destFile, format);
}

void LogManager::writeFilteredMessages(const QString& destFile, ExportFormat format)
{
    _exportEntries(_model->filteredEntries(), destFile, format);
}

void LogManager::_exportEntries(QList<LogEntry> entries, const QString& destFile, ExportFormat format)
{
    emit writeStarted();

    QPointer<LogManager> guard(this);
    _exportFuture = QtConcurrent::run([guard, destFile, entries = std::move(entries), format]() {
        bool success = false;
        QSaveFile file(destFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            const QByteArray content = LogFormatter::format(entries, format);
            file.write(content);
            success = file.commit();
        } else {
            qCWarning(LogManagerLog) << "write failed:" << file.errorString();
        }
        if (guard) {
            QMetaObject::invokeMethod(
                guard.data(),
                [guard, success]() {
                    if (guard) {
                        emit guard->writeFinished(success);
                    }
                },
                Qt::QueuedConnection);
        }
    });
}

// ---------------------------------------------------------------------------
// Test capture
// ---------------------------------------------------------------------------

void LogManager::setCaptureEnabled(bool enabled)
{
    s_captureEnabled.store(enabled, std::memory_order_relaxed);
}

void LogManager::clearCapturedMessages()
{
    const QMutexLocker locker(&s_captureMutex);
    s_capturedMessages.clear();
}

QList<LogEntry> LogManager::capturedMessages(const QString& category)
{
    const QMutexLocker locker(&s_captureMutex);

    if (category.isEmpty()) {
        return s_capturedMessages;
    }

    QList<LogEntry> filtered;
    for (const auto& msg : std::as_const(s_capturedMessages)) {
        if (msg.category == category) {
            filtered.append(msg);
        }
    }
    return filtered;
}

bool LogManager::hasCapturedMessage(const QString& category, LogEntry::Level level)
{
    const QMutexLocker locker(&s_captureMutex);
    for (const auto& msg : std::as_const(s_capturedMessages)) {
        if (msg.category == category && msg.level == level) {
            return true;
        }
    }
    return false;
}

bool LogManager::hasCapturedWarning(const QString& category)
{
    return hasCapturedMessage(category, LogEntry::Warning);
}

bool LogManager::hasCapturedCritical(const QString& category)
{
    return hasCapturedMessage(category, LogEntry::Critical);
}

bool LogManager::hasCapturedUncategorizedMessage()
{
    const QMutexLocker locker(&s_captureMutex);
    for (const auto& msg : std::as_const(s_capturedMessages)) {
        if (msg.category.isEmpty() || msg.category == QStringLiteral("default")) {
            return true;
        }
    }
    return false;
}

void LogManager::captureIfEnabled(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if (!s_captureEnabled.load(std::memory_order_relaxed)) {
        return;
    }

    LogEntry entry = buildEntry(type, context, msg);

    const QMutexLocker locker(&s_captureMutex);
    s_capturedMessages.append(std::move(entry));
}

LogEntry LogManager::buildEntry(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.level = LogEntry::fromQtMsgType(type);
    entry.category = context.category ? QString::fromLatin1(context.category) : QString();
    entry.message = message;
    if (context.file) {
        const QString fullPath = QString::fromLatin1(context.file);
        const int lastSlash = fullPath.lastIndexOf(QLatin1Char('/'));
        entry.file = (lastSlash >= 0) ? fullPath.mid(lastSlash + 1) : fullPath;
    }
    entry.function = context.function ? QString::fromLatin1(context.function) : QString();
    entry.line = context.line;
    entry.threadId = QThread::currentThreadId();
    entry.buildFormatted();
    return entry;
}
