#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QWaitCondition>

#include <atomic>
#include <functional>

class QThread;

class QGCFileWriter : public QObject
{
    Q_OBJECT

public:
    explicit QGCFileWriter(QObject *parent = nullptr);
    ~QGCFileWriter() override;

    void setFilePath(const QString &path);
    QString filePath() const;

    bool isRunning() const;
    bool isOpen() const { return _isOpen.load(std::memory_order_relaxed); }
    bool hasError() const { return _hasError.load(std::memory_order_relaxed); }
    QString lastError() const;

    qint64 fileSize() const { return _fileSize.load(std::memory_order_relaxed); }
    qint64 pendingBytes() const { return _pendingBytes.load(std::memory_order_relaxed); }

    using FormatFunc = std::function<QByteArray()>;

    void write(const QByteArray &data);
    void writeDeferred(FormatFunc formatter);
    /// Waits until everything queued so far is flushed to the file. Returns false on timeout or error.
    bool flush(int timeoutMs = 5000);
    /// Writes what is queued and stops the writer. A write that fails here is an error like any other.
    void close();
    /// An error drops what is queued and stops the writer. Clearing it, or writing again, starts it anew.
    void clearError();

    void setMaxPendingBytes(qint64 max) { _maxPendingBytes = max; }
    qint64 maxPendingBytes() const { return _maxPendingBytes; }

signals:
    void errorOccurred(const QString &message);
    void fileSizeChanged(qint64 size);

private:
    void _workerLoop();
    void _startLocked();
    void _stop();
    void _setError(const QString& message);
    void _clearErrorLocked();

    struct WorkItem {
        QByteArray data;
        FormatFunc formatter;
    };

    mutable QMutex _mutex;
    QMutex _stopMutex;  ///< Held by _stop() from start to end. Locked before _mutex.
    QWaitCondition _condition;
    QWaitCondition _flushedCondition;
    QList<WorkItem> _queue;
    // Item counts, guarded by _mutex. An item is "written" once it is flushed to the file or dropped on error.
    quint64 _queuedCount = 0;
    quint64 _writtenCount = 0;

    QString _filePath;
    QString _lastError;

    QThread *_thread = nullptr;
    // Guarded by _mutex. While set, _thread belongs to _stop(): nothing else joins, deletes or replaces it.
    bool _stopping = false;
    std::atomic<bool> _isOpen{false};
    std::atomic<bool> _hasError{false};
    std::atomic<bool> _quit{false};
    std::atomic<qint64> _fileSize{0};
    std::atomic<qint64> _pendingBytes{0};
    qint64 _maxPendingBytes = 50LL * 1024 * 1024;
};
