/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>
#include <QtCore/QStringListModel>
#include <QtCore/QTimer>

Q_DECLARE_LOGGING_CATEGORY(QGCLoggingLog)

class QGCLogging : public QStringListModel
{
    Q_OBJECT

public:
    explicit QGCLogging(QObject *parent = nullptr);

    /// Get the singleton instance
    static QGCLogging *instance();

    /// Install Qt message handler to route logs through this class
    static void installHandler();

    /// Write current log messages to a file asynchronously
    Q_INVOKABLE void writeMessages(const QString &destFile);

    /// Enqueue a log message (thread-safe)
    void log(const QString &message);

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
