#pragma once

#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/QStringListModel>
#include <QtCore/QTimer>

Q_DECLARE_LOGGING_CATEGORY(QGCLoggingLog)

/// A single captured log message for test introspection.
struct CapturedLogMessage
{
    QtMsgType type = QtDebugMsg;
    QString category;
    QString message;
};

class QGCLogging : public QStringListModel
{
    Q_OBJECT

public:
    explicit QGCLogging(QObject *parent = nullptr);
    ~QGCLogging();

    /// Get the singleton instance
    static QGCLogging *instance();

    /// Install Qt message handler to route logs through this class
    static void installHandler();

    /// Write current log messages to a file asynchronously
    Q_INVOKABLE void writeMessages(const QString &destFile);

    /// Enqueue a log message (thread-safe)
    void log(const QString &message);

    // ========================================================================
    // Test Log Capture
    // ========================================================================

    /// Enable or disable log capture (for unit testing).
    /// When enabled, every message passing through the Qt message handler is
    /// recorded with its category, type and raw text.
    static void setCaptureEnabled(bool enabled);

    /// Discard all previously captured messages.
    static void clearCapturedMessages();

    /// Return captured messages, optionally filtered to a single category.
    /// Pass an empty string to retrieve all messages.
    [[nodiscard]] static QList<CapturedLogMessage> capturedMessages(const QString &category = {});

    /// Return true if at least one message of the given @a type was captured
    /// for @a category.
    [[nodiscard]] static bool hasCapturedMessage(const QString &category, QtMsgType type);

    /// Convenience: captured warning in @a category?
    [[nodiscard]] static bool hasCapturedWarning(const QString &category);

    /// Convenience: captured critical in @a category?
    [[nodiscard]] static bool hasCapturedCritical(const QString &category);

    /// Return true if any uncategorized message was captured (e.g. raw qDebug/qWarning).
    [[nodiscard]] static bool hasCapturedUncategorizedMessage();

    /// Record a message into the capture buffer (if capture is enabled).
    /// Intended for external handler wrappers (e.g. the unit-test capture handler).
    static void captureIfEnabled(QtMsgType type, const QMessageLogContext &context, const QString &msg);

signals:
    /// Emitted when a log message is enqueued
    void emitLog(const QString &message);

    /// Emitted when file write starts
    void writeStarted();

    /// Emitted when file write finishes (success flag)
    void writeFinished(bool success);

private slots:
    /// Internal slot to append message on the GUI/main thread
    void _threadsafeLog(const QString &message);

    /// Periodic flush of pending log lines to disk
    void _flushToDisk();

private:
    void _rotateLogs();

    QFile _logFile;
    QTimer _flushTimer;
    QStringList _pendingDiskWrites;
    bool _ioError = false;

    static constexpr int kMaxLogFileSize = 10LL * 1024 * 1024;
    static constexpr int kMaxBackupFiles = 5;
    static constexpr int kFlushIntervalMSecs = 1000;
};
