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
    bool flush(int timeoutMs = 5000);
    void close();
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

    struct WorkItem {
        QByteArray data;
        FormatFunc formatter;
    };

    mutable QMutex _mutex;
    QWaitCondition _condition;
    QList<WorkItem> _queue;

    QString _filePath;
    QString _lastError;

    QThread *_thread = nullptr;
    std::atomic<bool> _isOpen{false};
    std::atomic<bool> _hasError{false};
    std::atomic<bool> _quit{false};
    std::atomic<qint64> _fileSize{0};
    std::atomic<qint64> _pendingBytes{0};
    std::atomic<quint64> _flushSeq{0};
    std::atomic<quint64> _writeSeq{0};
    qint64 _maxPendingBytes = 50LL * 1024 * 1024;
};
