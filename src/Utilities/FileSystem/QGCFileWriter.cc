#include "QGCFileWriter.h"
#include "QGCFileHelper.h"
#include <QtCore/QLoggingCategory>

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QMetaObject>
#include <QtCore/QThread>

Q_STATIC_LOGGING_CATEGORY(QGCFileWriterLog, "Utilities.FileSystem.QGCFileWriter", QtWarningMsg)

QGCFileWriter::QGCFileWriter(QObject* parent)
    : QObject(parent)
{
}

QGCFileWriter::~QGCFileWriter()
{
    stop();
}

void QGCFileWriter::setFilePath(const QString& path)
{
    bool needReopen = false;
    {
        QMutexLocker locker(&_mutex);
        const QString effectivePath = (_running && _reopenRequested) ? _nextFilePath : _filePath;
        if (effectivePath == path) {
            return;
        }
        if (_running) {
            _nextFilePath = path;
            _reopenRequested = true;
            needReopen = true;
        } else {
            _filePath = path;
            _nextFilePath.clear();
        }
    }

    if (needReopen) {
        _cv.wakeOne();
    }
}

QString QGCFileWriter::filePath() const
{
    QMutexLocker locker(&_mutex);
    if (_running && _reopenRequested && !_nextFilePath.isEmpty()) {
        return _nextFilePath;
    }
    return _filePath;
}

bool QGCFileWriter::isRunning() const
{
    QMutexLocker locker(&_mutex);
    return _running;
}

bool QGCFileWriter::isOpen() const
{
    QMutexLocker locker(&_mutex);
    return _file.isOpen();
}

qint64 QGCFileWriter::fileSize() const
{
    QMutexLocker locker(&_mutex);
    return _file.isOpen() ? _file.size() : -1;
}

void QGCFileWriter::write(const QByteArray& data)
{
    if (data.isEmpty()) {
        return;
    }

    {
        QMutexLocker locker(&_mutex);
        if (!_running) {
            return;
        }

        _queue.enqueue(data);
        _pendingBytes += data.size();

        // Drop oldest data if over limit
        if (_pendingBytes > _maxPendingBytes) {
            qCWarning(QGCFileWriterLog) << "Write buffer full, dropping oldest data";
            while (_pendingBytes > _maxPendingBytes && !_queue.isEmpty()) {
                _pendingBytes -= _queue.dequeue().size();
            }
        }
    }

    _cv.wakeOne();
}

void QGCFileWriter::write(const QString& text)
{
    write(text.toUtf8());
}

bool QGCFileWriter::flush(int timeoutMs)
{
    {
        QMutexLocker locker(&_mutex);
        if (!_running) {
            return false;
        }
        _flushRequested = true;
    }

    _cv.wakeOne();

    QMutexLocker locker(&_mutex);
    while (_flushRequested && _running) {
        if (!_flushCv.wait(&_mutex, timeoutMs)) {
            return false;
        }
    }
    return true;
}

void QGCFileWriter::start()
{
    {
        QMutexLocker locker(&_mutex);
        if (_running) {
            return;
        }
        _running = true;
    }

    _future = QtConcurrent::run(&QGCFileWriter::_writerLoop, this);
    qCDebug(QGCFileWriterLog) << "Writer started via QtConcurrent";
}

void QGCFileWriter::stop()
{
    {
        QMutexLocker locker(&_mutex);
        if (!_running) {
            return;
        }
        _running = false;
    }

    _cv.wakeOne();

    if (_future.isValid()) {
        _future.waitForFinished();
    }

    _closeFile();
    qCDebug(QGCFileWriterLog) << "Writer stopped";
}

void QGCFileWriter::close()
{
    bool running = false;
    {
        QMutexLocker locker(&_mutex);
        running = _running;
        if (running) {
            _closeRequested = true;
        }
    }

    if (running) {
        _cv.wakeOne();
    } else {
        _closeFile();
    }
}

bool QGCFileWriter::hasError() const
{
    QMutexLocker locker(&_mutex);
    return _hasError;
}

void QGCFileWriter::clearError()
{
    {
        QMutexLocker locker(&_mutex);
        if (!_hasError) {
            return;
        }
        _hasError = false;
    }

    QMetaObject::invokeMethod(this, [this]() {
        emit errorCleared();
    }, Qt::QueuedConnection);
}

void QGCFileWriter::setMaxPendingBytes(qint64 bytes)
{
    QMutexLocker locker(&_mutex);
    _maxPendingBytes = bytes;
}

qint64 QGCFileWriter::maxPendingBytes() const
{
    QMutexLocker locker(&_mutex);
    return _maxPendingBytes;
}

void QGCFileWriter::setPreOpenCallback(PreOpenCallback callback)
{
    QMutexLocker locker(&_mutex);
    _preOpenCallback = std::move(callback);
}

void QGCFileWriter::setFileSizeCallback(FileSizeCallback callback)
{
    QMutexLocker locker(&_mutex);
    _fileSizeCallback = std::move(callback);
}

void QGCFileWriter::_writerLoop()
{
    QQueue<QByteArray> batch;
    qint64 lastReportedSize = 0;
    constexpr qint64 kSizeReportThreshold = 1024 * 1024; // Report every 1MB
    auto requeueBatch = [this](QQueue<QByteArray>& queue) {
        if (queue.isEmpty()) {
            return;
        }

        QMutexLocker locker(&_mutex);
        while (!queue.isEmpty()) {
            const QByteArray data = queue.takeLast();
            _queue.prepend(data);
            _pendingBytes += data.size();
        }
    };

    while (true) {
        // Wait for data or signal
        {
            QMutexLocker locker(&_mutex);

            while (_queue.isEmpty() && !_flushRequested && !_closeRequested &&
                   !_reopenRequested && _running) {
                _cv.wait(&_mutex, kFlushTimeoutMs);
            }

            if (!_running && _queue.isEmpty()) {
                break;
            }

            batch.swap(_queue);
            _pendingBytes = 0;
        }

        if (!batch.isEmpty()) {
            bool fileOpen = false;
            {
                QMutexLocker locker(&_mutex);
                fileOpen = _file.isOpen();
            }

            if (!fileOpen && !_openFile()) {
                requeueBatch(batch);
                QThread::msleep(50);
                continue;
            }

            while (!batch.isEmpty()) {
                const QByteArray data = batch.dequeue();
                qint64 totalWritten = 0;
                bool writeFailed = false;

                while (totalWritten < data.size()) {
                    qint64 written = 0;
                    {
                        QMutexLocker locker(&_mutex);
                        written = _file.write(data.constData() + totalWritten, data.size() - totalWritten);
                    }
                    if (written < 0) {
                        QString errorString;
                        {
                            QMutexLocker locker(&_mutex);
                            errorString = _file.errorString();
                        }
                        _emitError(QStringLiteral("Write failed: %1").arg(errorString));
                        writeFailed = true;
                        break;
                    }
                    totalWritten += written;
                }

                if (writeFailed) {
                    _closeFile();
                    if (totalWritten == 0) {
                        QMutexLocker locker(&_mutex);
                        _queue.prepend(data);
                        _pendingBytes += data.size();
                    }
                    // Re-queue remaining items so they aren't silently dropped
                    requeueBatch(batch);
                    break;
                }
            }

            // Report size changes
            qint64 currentSize = 0;
            FileSizeCallback callback;
            {
                QMutexLocker locker(&_mutex);
                currentSize = _file.size();
                callback = _fileSizeCallback;
            }

            if (currentSize - lastReportedSize >= kSizeReportThreshold) {
                lastReportedSize = currentSize;
                if (callback) {
                    callback(currentSize);
                }
            }
        }
        batch.clear();

        // Handle reopen/close requests after current batch is drained.
        bool needClose = false;
        {
            QMutexLocker locker(&_mutex);
            if (_closeRequested || _reopenRequested) {
                needClose = true;
                _closeRequested = false;
                if (_reopenRequested) {
                    _filePath = _nextFilePath;
                    _nextFilePath.clear();
                    _reopenRequested = false;
                }
            }
        }
        if (needClose) {
            _closeFile();
        }

        // Handle flush request
        {
            QMutexLocker locker(&_mutex);
            if (_flushRequested) {
                if (_file.isOpen()) {
                    _file.flush();
                }
                _flushRequested = false;
                _flushCv.wakeAll();
            }
        }
    }

    // Final flush on shutdown
    {
        QMutexLocker locker(&_mutex);
        batch.swap(_queue);
        _pendingBytes = 0;
    }

    if (!batch.isEmpty()) {
        QMutexLocker locker(&_mutex);
        if (_file.isOpen()) {
            while (!batch.isEmpty()) {
                _file.write(batch.dequeue());
            }
        }
    }

    {
        QMutexLocker locker(&_mutex);
        if (_file.isOpen()) {
            _file.flush();
        }
    }
}

bool QGCFileWriter::_openFile()
{
    QString path;
    PreOpenCallback callback;
    {
        QMutexLocker locker(&_mutex);
        path = _filePath;
        callback = _preOpenCallback;
    }

    if (path.isEmpty()) {
        return false;
    }

    // Invoke pre-open callback
    if (callback && !callback(path)) {
        return false;
    }

    if (!QGCFileHelper::ensureParentExists(path)) {
        _emitError(QStringLiteral("Failed to create directory for: %1").arg(path));
        return false;
    }

    {
        QMutexLocker locker(&_mutex);
        _file.setFileName(path);
        if (!_file.open(QIODevice::WriteOnly | QIODevice::Append)) {
            _emitError(QStringLiteral("Failed to open file %1: %2").arg(path, _file.errorString()));
            return false;
        }
    }

    qCDebug(QGCFileWriterLog) << "Opened file:" << path;

    QMetaObject::invokeMethod(this, [this, path]() {
        emit fileOpened(path);
    }, Qt::QueuedConnection);

    return true;
}

void QGCFileWriter::_closeFile()
{
    bool wasOpen = false;
    {
        QMutexLocker locker(&_mutex);
        wasOpen = _file.isOpen();
        if (wasOpen) {
            _file.flush();
            _file.close();
        }
    }
    if (wasOpen) {
        qCDebug(QGCFileWriterLog) << "Closed file";
        QMetaObject::invokeMethod(this, [this]() {
            emit fileClosed();
        }, Qt::QueuedConnection);
    }
}

void QGCFileWriter::_emitError(const QString& message)
{
    {
        QMutexLocker locker(&_mutex);
        _hasError = true;
    }
    qCWarning(QGCFileWriterLog) << message;

    QMetaObject::invokeMethod(this, [this, message]() {
        emit errorOccurred(message);
    }, Qt::QueuedConnection);
}
