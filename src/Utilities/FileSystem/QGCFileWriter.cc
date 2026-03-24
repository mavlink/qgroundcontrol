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
        _hasError.store(false, std::memory_order_relaxed);
        _lastError.clear();
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
        if (!_thread) {
            _startLocked();
        }
        _queue.append(WorkItem{data, {}});
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
        if (!_thread) {
            _startLocked();
        }
        _queue.append(WorkItem{{}, std::move(formatter)});
    }
    _condition.wakeOne();
}

bool QGCFileWriter::flush(int timeoutMs)
{
    QMutexLocker locker(&_mutex);
    if (!_thread) {
        return true;
    }

    const quint64 target = _flushSeq.fetch_add(1, std::memory_order_relaxed) + 1;
    _condition.wakeOne();

    const auto deadline = QDeadlineTimer(timeoutMs);
    while (_writeSeq.load(std::memory_order_acquire) < target
           && !_quit.load(std::memory_order_relaxed)) {
        if (!_condition.wait(&_mutex, deadline)) {
            return false;
        }
    }
    return true;
}

void QGCFileWriter::close()
{
    _stop();
}

void QGCFileWriter::clearError()
{
    const QMutexLocker locker(&_mutex);
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
    {
        const QMutexLocker locker(&_mutex);
        if (!_thread) {
            return;
        }
        _quit.store(true, std::memory_order_relaxed);
    }
    _condition.wakeOne();
    _thread->wait();
    _thread->deleteLater();

    const QMutexLocker locker(&_mutex);
    _thread = nullptr;
    _isOpen.store(false, std::memory_order_relaxed);
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
        const QMutexLocker locker(&_mutex);
        _lastError = file.errorString();
        _hasError.store(true, std::memory_order_relaxed);
        QMetaObject::invokeMethod(this, [this, err = _lastError]() {
            emit errorOccurred(err);
        }, Qt::QueuedConnection);
        return;
    }

    _isOpen.store(true, std::memory_order_relaxed);
    _fileSize.store(file.size(), std::memory_order_relaxed);

    while (!_quit.load(std::memory_order_relaxed)) {
        QList<WorkItem> batch;

        {
            QMutexLocker locker(&_mutex);
            while (_queue.isEmpty() && !_quit.load(std::memory_order_relaxed)) {
                _condition.wait(&_mutex);
            }
            batch.swap(_queue);
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
                const QMutexLocker locker(&_mutex);
                _lastError = file.errorString();
                _hasError.store(true, std::memory_order_relaxed);
                QMetaObject::invokeMethod(this, [this, err = _lastError]() {
                    emit errorOccurred(err);
                }, Qt::QueuedConnection);
                writeError = true;
                break;
            }
        }

        _pendingBytes.fetch_sub(pendingBytesConsumed, std::memory_order_relaxed);

        if (!writeError) {
            file.flush();
            _fileSize.store(file.size(), std::memory_order_relaxed);
            const qint64 sz = file.size();
            QMetaObject::invokeMethod(this, [this, sz]() {
                emit fileSizeChanged(sz);
            }, Qt::QueuedConnection);
        }

        _writeSeq.fetch_add(1, std::memory_order_release);
        _condition.wakeAll();

        if (writeError) {
            break;
        }
    }

    // Drain remaining queue
    {
        QMutexLocker locker(&_mutex);
        for (auto &item : _queue) {
            if (item.formatter) {
                item.data = item.formatter();
                item.formatter = nullptr;
            }
            if (!item.data.isEmpty()) {
                file.write(item.data);
            }
        }
        _pendingBytes.store(0, std::memory_order_relaxed);
        _queue.clear();
    }

    file.close();
    _isOpen.store(false, std::memory_order_relaxed);
    _fileSize.store(0, std::memory_order_relaxed);
    _writeSeq.fetch_add(1, std::memory_order_release);
    _condition.wakeAll();
}
