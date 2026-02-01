#pragma once

#include "QGCLogEntry.h"

#include <QtCore/QFuture>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QWaitCondition>

class QGCFileWriter;

/// Handles writing log entries to disk with rotation.
/// Uses QGCFileWriter for async I/O and QtConcurrent for processing.
class LogDiskWriter : public QObject
{
    Q_OBJECT

public:
    explicit LogDiskWriter(QObject* parent = nullptr);
    ~LogDiskWriter() override;

    /// Queues an entry for writing. Thread-safe, non-blocking.
    void write(const QGCLogEntry& entry);

    /// Sets the log file path. Must be called before enabling.
    void setFilePath(const QString& path);
    QString filePath() const;

    /// Enables or disables disk writing.
    void setEnabled(bool enabled);
    bool isEnabled() const;

    /// Forces an immediate flush of pending entries.
    void flush();

    /// Returns true if an I/O error has occurred.
    bool hasError() const;

    /// Clears the error state and attempts recovery.
    void clearError();

    /// Configuration
    void setMaxFileSize(qint64 bytes);
    void setMaxBackupFiles(int count);
    void setMaxPendingEntries(int count);

signals:
    void errorOccurred(const QString& message);
    void errorCleared();

private slots:
    void _onWriterError(const QString& message);

private:
    void _startProcessing();
    void _stopProcessing();
    void _processingLoop();
    void _rotateLogs();
    void _checkDiskSpace();
    bool _shouldRotate(qint64 fileSize) const;

    // File writer
    QGCFileWriter* _writer = nullptr;

    // Processing (QtConcurrent)
    QFuture<void> _future;
    mutable QMutex _mutex;
    QWaitCondition _cv;
    QQueue<QGCLogEntry> _queue; bool _running = false;
    bool _flushRequested = false;

    // Configuration (protected by _mutex)
    bool _enabled = false;
    bool _hasError = false;
    qint64 _maxFileSize = 10LL * 1024 * 1024;
    int _maxBackupFiles = 5;
    int _maxPendingEntries = 10000;

    static constexpr int kFlushTimeoutMs = 1000;
};
