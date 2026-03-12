#include "QGCLogging.h"
#include "AppSettings.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QGlobalStatic>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QStringListModel>
#include <QtCore/QTextStream>

#include <atomic>

#ifdef Q_OS_ANDROID
#include <android/log.h>
#endif

QGC_LOGGING_CATEGORY(QGCLoggingLog, "Utilities.QGCLogging")

Q_GLOBAL_STATIC(QGCLogging, _qgcLogging)

static QtMessageHandler defaultHandler = nullptr;

// ---------------------------------------------------------------------------
// Test‐log‐capture storage (thread‐safe, lightweight when disabled)
// ---------------------------------------------------------------------------
static std::atomic<bool> s_captureEnabled{false};
static QMutex s_captureMutex;
static QList<CapturedLogMessage> s_capturedMessages;

static void msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // Call the previous handler FIRST to ensure QTest::ignoreMessage works correctly.
    // QTest's message filtering happens in the default handler, so we must call it
    // before any processing that might interfere with message matching.
    if (defaultHandler) {
        defaultHandler(type, context, msg);
    }

#ifdef Q_OS_ANDROID
    // Qt 6.10+ may not route messages to logcat via the default handler.
    // Write directly to ensure visibility in adb logcat.
    {
        int prio;
        switch (type) {
        case QtDebugMsg:    prio = ANDROID_LOG_DEBUG; break;
        case QtInfoMsg:     prio = ANDROID_LOG_INFO;  break;
        case QtWarningMsg:  prio = ANDROID_LOG_WARN;  break;
        case QtCriticalMsg: prio = ANDROID_LOG_ERROR;  break;
        case QtFatalMsg:    prio = ANDROID_LOG_FATAL;  break;
        }
        __android_log_print(prio, context.category ? context.category : "QGC",
                            "%s", qPrintable(msg));
    }
#endif

    // Format the message using Qt's pattern
    const QString message = qFormatLogMessage(type, context, msg);

    // Filter out Qt Quick internals
    if (QGCLogging::instance() && !QString(context.category).startsWith("qt.quick")) {
        // Capture for unit-test introspection
        QGCLogging::captureIfEnabled(type, context, msg);

        QGCLogging::instance()->log(message);
    }
}

QGCLogging *QGCLogging::instance()
{
    return _qgcLogging();
}

QGCLogging::QGCLogging(QObject *parent)
    : QStringListModel(parent)
{
    qCDebug(QGCLoggingLog) << this;

    _flushTimer.setInterval(kFlushIntervalMSecs);
    _flushTimer.setSingleShot(false);
    (void) connect(&_flushTimer, &QTimer::timeout, this, &QGCLogging::_flushToDisk);
    _flushTimer.start();

    // Connect the emitLog signal to threadsafeLog slot
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    const Qt::ConnectionType conntype = Qt::QueuedConnection;
#else
    const Qt::ConnectionType conntype = Qt::AutoConnection;
#endif
    (void) connect(this, &QGCLogging::emitLog, this, &QGCLogging::_threadsafeLog, conntype);
}

QGCLogging::~QGCLogging()
{
    qCDebug(QGCLoggingLog) << this;
}

void QGCLogging::installHandler()
{
    // Define the format for qDebug/qWarning/etc output
    qSetMessagePattern(QStringLiteral("%{time process}%{if-warning} Warning:%{endif}%{if-critical} Critical:%{endif} %{message} - %{category} - (%{function}:%{line})"));

    // Install our custom handler
    defaultHandler = qInstallMessageHandler(msgHandler);
}

// ---------------------------------------------------------------------------
// Test‑log‑capture API
// ---------------------------------------------------------------------------

void QGCLogging::setCaptureEnabled(bool enabled)
{
    s_captureEnabled.store(enabled, std::memory_order_relaxed);
}

void QGCLogging::clearCapturedMessages()
{
    const QMutexLocker locker(&s_captureMutex);
    s_capturedMessages.clear();
}

QList<CapturedLogMessage> QGCLogging::capturedMessages(const QString &category)
{
    const QMutexLocker locker(&s_captureMutex);

    if (category.isEmpty()) {
        return s_capturedMessages;
    }

    QList<CapturedLogMessage> filtered;
    for (const auto &msg : std::as_const(s_capturedMessages)) {
        if (msg.category == category) {
            filtered.append(msg);
        }
    }
    return filtered;
}

bool QGCLogging::hasCapturedMessage(const QString &category, QtMsgType type)
{
    const QMutexLocker locker(&s_captureMutex);

    for (const auto &msg : std::as_const(s_capturedMessages)) {
        if (msg.category == category && msg.type == type) {
            return true;
        }
    }
    return false;
}

bool QGCLogging::hasCapturedWarning(const QString &category)
{
    return hasCapturedMessage(category, QtWarningMsg);
}

bool QGCLogging::hasCapturedCritical(const QString &category)
{
    return hasCapturedMessage(category, QtCriticalMsg);
}

bool QGCLogging::hasCapturedUncategorizedMessage()
{
    const QMutexLocker locker(&s_captureMutex);

    for (const auto &msg : std::as_const(s_capturedMessages)) {
        if (msg.category.isEmpty() || msg.category == QStringLiteral("default")) {
            return true;
        }
    }
    return false;
}

void QGCLogging::captureIfEnabled(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (!s_captureEnabled.load(std::memory_order_relaxed)) {
        return;
    }

    const QMutexLocker locker(&s_captureMutex);
    s_capturedMessages.append({type,
                               context.category ? QString::fromLatin1(context.category) : QString(),
                               msg});
}

void QGCLogging::log(const QString &message)
{
    // Emit the signal so threadsafeLog runs in the correct thread
    if (!_ioError) {
        emit emitLog(message);
    }
}

void QGCLogging::_threadsafeLog(const QString &message)
{
    // Notify view of new row
    const int line = rowCount();
    (void) QStringListModel::insertRows(line, 1);
    (void) setData(index(line, 0), message, Qt::DisplayRole);

    // Trim old entries to cap memory usage
    static constexpr const int kMaxLogRows = kMaxLogFileSize / 100;
    if (rowCount() > kMaxLogRows) {
        const int removeCount = rowCount() - kMaxLogRows;
        beginRemoveRows(QModelIndex(), 0, removeCount - 1);
        (void) removeRows(0, removeCount);
        endRemoveRows();
    }

    // Queue for disk flush
    _pendingDiskWrites.append(message);
}

void QGCLogging::_rotateLogs()
{
    // Close the current log
    _logFile.close();

    // Full path without extension
    const QString basePath = _logFile.fileName();    // e.g. "/path/QGCConsole.log"
    const QFileInfo fileInfo(basePath);
    const QString dir = fileInfo.absolutePath();
    const QString name = fileInfo.baseName();        // "QGCConsole"
    const QString ext = fileInfo.completeSuffix();   // "log"

    // Rotate existing backups: QGCConsole.4.log → QGCConsole.5.log, …
    for (int i = kMaxBackupFiles - 1; i >= 1; --i) {
        const QString from = QStringLiteral("%1/%2.%3.%4").arg(dir, name).arg(i).arg(ext);
        const QString to = QStringLiteral("%1/%2.%3.%4").arg(dir, name).arg(i+1).arg(ext);
        if (QFile::exists(to)) {
            (void) QFile::remove(to);
        }
        if (QFile::exists(from)) {
            (void) QFile::rename(from, to);
        }
    }

    // Move the just‐closed log to “.1”
    const QString firstBackup = QStringLiteral("%1/%2.1.%3").arg(dir, name, ext);
    if (QFile::exists(firstBackup)) {
        (void) QFile::remove(firstBackup);
    }
    (void) QFile::rename(basePath, firstBackup);

    // Re‑open a fresh log file
    _logFile.setFileName(basePath);
    if (!_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        _ioError = true;
        qgcApp()->showAppMessage(tr("Unable to reopen log file %1: %2").arg(_logFile.fileName(), _logFile.errorString()));
    }
}

void QGCLogging::_flushToDisk()
{
    if (_pendingDiskWrites.isEmpty() || _ioError) {
        return;
    }

    // Ensure log output enabled and file open
    if (!_logFile.isOpen()) {
        if (!qgcApp()->logOutput()) {
            _pendingDiskWrites.clear();
            return;
        }

        const QString saveDirPath = SettingsManager::instance()->appSettings()->crashSavePath();
        const QDir saveDir(saveDirPath);
        const QString saveFilePath = saveDir.absoluteFilePath("QGCConsole.log");

        _logFile.setFileName(saveFilePath);
        if (!_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            _ioError = true;
            qgcApp()->showAppMessage(tr("Open console log output file failed %1 : %2").arg(_logFile.fileName(), _logFile.errorString()));
            return;
        }
    }

    // Check size before writing
    if (_logFile.size() >= kMaxLogFileSize) {
        _rotateLogs();
    }

    // Write all pending lines
    QTextStream out(&_logFile);
    for (const QString &line : std::as_const(_pendingDiskWrites)) {
        out << line << '\n';
        if (out.status() != QTextStream::Ok) {
            _ioError = true;
            qCWarning(QGCLoggingLog) << "Error writing to log file:" << _logFile.errorString();
            break;
        }
    }
    (void) _logFile.flush();
    _pendingDiskWrites.clear();
}

void QGCLogging::writeMessages(const QString &destFile)
{
    // Snapshot current logs on GUI thread
    const QStringList logs = stringList();

    // Run the file write in a separate thread
    (void) QtConcurrent::run([this, destFile, logs]() {
        emit writeStarted();
        bool success = false;
        QSaveFile file(destFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            for (const QString &line : logs) {
                out << line << '\n';
            }
            success = ((out.status() == QTextStream::Ok) && file.commit());
        } else {
            qCWarning(QGCLoggingLog) << "write failed:" << file.errorString();
        }
        emit writeFinished(success);
    });
}
