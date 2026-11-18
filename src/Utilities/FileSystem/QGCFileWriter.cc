#include "QGCFileWriter.h"

#include <QtCore/QFile>
#include <QtCore/QMutexLocker>
#include <QtCore/QThread>

QGCFileWriter::QGCFileWriter(QObject *parent)
    : QObject(parent)
{
}

QGCFileWriter::~QGCFileWriter()
{
    _stop();
}

void QGCFileWriter::setFilePath(const QString &path)
{
    bool wasRunning = false;
    {
        const QMutexLocker locker(&_mutex);
        if (_filePath == path) {
            return;
        }
        wasRunning = _thread != nullptr;
    }

    if (wasRunning) {
        _stop();
    }

    {
        const QMutexLocker locker(&_mutex);
        _filePath = path;
        _clearErrorLocked();
    }

    if (wasRunning && !path.isEmpty()) {
        const QMutexLocker locker(&_mutex);
        _startLocked();
    }
}

QString QGCFileWriter::filePath() const
{
    const QMutexLocker locker(&_mutex);
    return _filePath;
}

bool QGCFileWriter::isRunning() const
{
    const QMutexLocker locker(&_mutex);
    return _thread != nullptr && _thread->isRunning();
}

QString QGCFileWriter::lastError() const
{
    const QMutexLocker locker(&_mutex);
    return _lastError;
}

void QGCFileWriter::write(const QByteArray &data)
{
    if (data.isEmpty()) {
        return;
    }

    if (_pendingBytes.load(std::memory_order_relaxed) > _maxPendingBytes) {
        return;
    }

    {
        const QMutexLocker locker(&_mutex);
        if (_filePath.isEmpty()) {
            return;
        }
        _clearErrorLocked();
        if (!_thread) {
            _startLocked();
        }
        _queue.append(WorkItem{data, {}});
        ++_queuedCount;
        _pendingBytes.fetch_add(data.size(), std::memory_order_relaxed);
    }
    _condition.wakeOne();
}

void QGCFileWriter::writeDeferred(FormatFunc formatter)
{
    if (!formatter) {
        return;
    }

    {
        QMutexLocker locker(&_mutex);
        if (_filePath.isEmpty()) {
            return;
        }
        _clearErrorLocked();
        if (!_thread) {
            _startLocked();
        }
        _queue.append(WorkItem{{}, std::move(formatter)});
        ++_queuedCount;
    }
    _condition.wakeOne();
}

bool QGCFileWriter::flush(int timeoutMs)
{
    QMutexLocker locker(&_mutex);

    const quint64 target = _queuedCount;
    const auto deadline = QDeadlineTimer(timeoutMs);
    while (_writtenCount < target) {
        if (!_flushedCondition.wait(&_mutex, deadline)) {
            return false;
        }
    }
    return !_hasError.load(std::memory_order_relaxed);
}

void QGCFileWriter::close()
{
    _stop();
}

void QGCFileWriter::clearError()
{
    const QMutexLocker locker(&_mutex);
    _clearErrorLocked();
}

void QGCFileWriter::_clearErrorLocked()
{
    if (!_hasError.load(std::memory_order_relaxed)) {
        return;
    }

    // The failed worker is exiting and no longer locks the mutex (see _setError()), so it can be joined here.
    // That lets the next write start a new one. A worker which _stop() is waiting for is left to _stop().
    if (_thread && !_stopping) {
        _thread->wait();
        _thread->deleteLater();
        _thread = nullptr;
    }
    _hasError.store(false, std::memory_order_relaxed);
    _lastError.clear();
}

void QGCFileWriter::_startLocked()
{
    if (_thread) {
        return;
    }

    _quit.store(false, std::memory_order_relaxed);
    _thread = QThread::create([this]() { _workerLoop(); });
    _thread->setObjectName(QStringLiteral("QGCFileWriter"));
    _thread->start();
}

void QGCFileWriter::_stop()
{
    // One caller at a time: the worker is joined without _mutex, which it needs to write what is queued
    const QMutexLocker stopLocker(&_stopMutex);

    QThread* thread = nullptr;
    {
        const QMutexLocker locker(&_mutex);
        if (!_thread) {
            return;
        }
        thread = _thread;
        _stopping = true;
        _quit.store(true, std::memory_order_relaxed);
    }
    _condition.wakeOne();
    thread->wait();

    const QMutexLocker locker(&_mutex);
    thread->deleteLater();
    _thread = nullptr;
    _stopping = false;
    _isOpen.store(false, std::memory_order_relaxed);
}

// The worker's last action before it exits: it must not lock the mutex afterwards (see _clearErrorLocked()).
void QGCFileWriter::_setError(const QString& message)
{
    {
        const QMutexLocker locker(&_mutex);
        _lastError = message;
        // What is queued would never be written
        _queue.clear();
        _pendingBytes.store(0, std::memory_order_relaxed);
        _writtenCount = _queuedCount;
        _hasError.store(true, std::memory_order_relaxed);
    }
    _flushedCondition.wakeAll();

    QMetaObject::invokeMethod(this, [this, message]() { emit errorOccurred(message); }, Qt::QueuedConnection);
}

void QGCFileWriter::_workerLoop()
{
    QString path;
    {
        const QMutexLocker locker(&_mutex);
        path = _filePath;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        _setError(file.errorString());
        return;
    }

    _isOpen.store(true, std::memory_order_relaxed);
    _fileSize.store(file.size(), std::memory_order_relaxed);

    while (!_quit.load(std::memory_order_relaxed)) {
        QList<WorkItem> batch;
        quint64 batchEnd = 0;

        {
            QMutexLocker locker(&_mutex);
            while (_queue.isEmpty() && !_quit.load(std::memory_order_relaxed)) {
                _condition.wait(&_mutex);
            }
            batch.swap(_queue);
            batchEnd = _queuedCount;
        }

        // Resolve deferred items; only track bytes that were counted in _pendingBytes
        qint64 pendingBytesConsumed = 0;
        for (auto &item : batch) {
            if (item.formatter) {
                item.data = item.formatter();
                item.formatter = nullptr;
            } else {
                pendingBytesConsumed += item.data.size();
            }
        }

        bool writeError = false;
        for (const auto &item : std::as_const(batch)) {
            if (item.data.isEmpty()) {
                continue;
            }
            const qint64 written = file.write(item.data);
            if (written < 0) {
                writeError = true;
                break;
            }
        }

        _pendingBytes.fetch_sub(pendingBytesConsumed, std::memory_order_relaxed);

        // QFile buffers small writes, so their errors (disk full) only show up in flush()
        if (writeError || !file.flush()) {
            const QString error = file.errorString();
            file.close();
            _isOpen.store(false, std::memory_order_relaxed);
            _fileSize.store(0, std::memory_order_relaxed);
            _setError(error);
            return;
        }

        _fileSize.store(file.size(), std::memory_order_relaxed);
        const qint64 sz = file.size();
        QMetaObject::invokeMethod(this, [this, sz]() { emit fileSizeChanged(sz); }, Qt::QueuedConnection);

        {
            const QMutexLocker locker(&_mutex);
            _writtenCount = batchEnd;
        }
        _flushedCondition.wakeAll();
    }

    // Drain remaining queue
    bool drainError = false;
    QString drainErrorString;
    {
        QMutexLocker locker(&_mutex);
        for (auto &item : _queue) {
            if (item.formatter) {
                item.data = item.formatter();
                item.formatter = nullptr;
            }
            if (!item.data.isEmpty() && !drainError) {
                drainError = file.write(item.data) < 0;
            }
        }
        _pendingBytes.store(0, std::memory_order_relaxed);
        _queue.clear();
        // As above, the errors of buffered writes only show up in flush()
        if (drainError || !file.flush()) {
            drainError = true;
            drainErrorString = file.errorString();
        }
        file.close();
        _writtenCount = _queuedCount;
    }

    _isOpen.store(false, std::memory_order_relaxed);
    _fileSize.store(0, std::memory_order_relaxed);
    if (drainError) {
        _setError(drainErrorString);
        return;
    }
    _flushedCondition.wakeAll();
}
