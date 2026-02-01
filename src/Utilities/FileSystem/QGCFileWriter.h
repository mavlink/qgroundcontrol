#pragma once

#include <QtCore/QFile>
#include <QtCore/QFuture>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QWaitCondition>

#include <functional>

/// Async file writer with QtConcurrent.
/// Provides non-blocking writes with automatic buffering.
class QGCFileWriter : public QObject
{
    Q_OBJECT

public:
    explicit QGCFileWriter(QObject* parent = nullptr);
    ~QGCFileWriter() override;

    /// Sets the file path. Can be changed while running (will reopen).
    void setFilePath(const QString& path);
    QString filePath() const;

    /// Returns true if the writer is active.
    bool isRunning() const;

    /// Returns true if the file is currently open.
    bool isOpen() const;

    /// Returns current file size (-1 if not open).
    qint64 fileSize() const;

    /// Queues data for writing. Thread-safe, non-blocking.
    void write(const QByteArray& data);
    void write(const QString& text);

    /// Forces immediate flush of pending data.
    /// Blocks until flush completes or timeout.
    bool flush(int timeoutMs = 500);

    /// Starts the writer.
    void start();

    /// Stops the writer and flushes pending data.
    void stop();

    /// Closes the current file (data can still be queued).
    void close();

    /// Returns true if an I/O error has occurred.
    bool hasError() const;

    /// Clears the error state.
    void clearError();

    /// Configuration
    void setMaxPendingBytes(qint64 bytes);
    qint64 maxPendingBytes() const;

    /// Callback invoked before file is opened. Return false to abort.
    using PreOpenCallback = std::function<bool(const QString& path)>;
    void setPreOpenCallback(PreOpenCallback callback);

    /// Callback invoked when file size changes significantly.
    using FileSizeCallback = std::function<void(qint64 size)>;
    void setFileSizeCallback(FileSizeCallback callback);

signals:
    void errorOccurred(const QString& message);
    void errorCleared();
    void fileOpened(const QString& path);
    void fileClosed();

private:
    void _writerLoop();
    bool _openFile();
    void _closeFile();
    void _emitError(const QString& message);

    // Processing (QtConcurrent)
    QFuture<void> _future;
    mutable QMutex _mutex;
    QWaitCondition _cv;
    QQueue<QByteArray> _queue;
    bool _running = false;
    bool _flushRequested = false;
    bool _closeRequested = false;
    bool _reopenRequested = false;

    // File state (protected by _mutex)
    QFile _file;
    QString _filePath;

    // State (protected by _mutex)
    bool _hasError = false;
    qint64 _pendingBytes = 0;

    // Configuration (protected by _mutex)
    qint64 _maxPendingBytes = 50LL * 1024 * 1024; // 50MB default

    // Callbacks (protected by _mutex)
    PreOpenCallback _preOpenCallback;
    FileSizeCallback _fileSizeCallback;

    static constexpr int kFlushTimeoutMs = 1000;
};
