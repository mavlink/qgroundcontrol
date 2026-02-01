#include "LogDiskWriter.h"
#include "QGCFileHelper.h"
#include "QGCFileWriter.h"
#include <QtCore/QLoggingCategory>

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaObject>

Q_STATIC_LOGGING_CATEGORY(LogDiskWriterLog, "Utilities.Logging.LogDiskWriter", QtWarningMsg)

static constexpr qint64 kMinDiskSpaceBytes = 10 * 1024 * 1024; // 10MB minimum free space

LogDiskWriter::LogDiskWriter(QObject* parent)
    : QObject(parent)
    , _writer(new QGCFileWriter(this))
{
    connect(_writer, &QGCFileWriter::errorOccurred, this, &LogDiskWriter::_onWriterError);
    connect(_writer, &QGCFileWriter::errorCleared, this, &LogDiskWriter::errorCleared);

    // Set up file size callback for rotation
    _writer->setFileSizeCallback([this](qint64 size) {
        if (_shouldRotate(size)) {
            // Request rotation on next processing cycle
            _cv.wakeOne();
        }
    });
}

LogDiskWriter::~LogDiskWriter()
{
    _stopProcessing();
}

void LogDiskWriter::setFilePath(const QString& path)
{
    _writer->setFilePath(path);
}

QString LogDiskWriter::filePath() const
{
    return _writer->filePath();
}

void LogDiskWriter::setEnabled(bool enabled)
{
    {
        QMutexLocker locker(&_mutex);
        if (_enabled == enabled) {
            return;
        }
        _enabled = enabled;
    }

    if (enabled) {
        _writer->start();
        _startProcessing();
    } else {
        _stopProcessing();
        _writer->stop();
        QMutexLocker locker(&_mutex);
        _queue.clear();
    }
}

bool LogDiskWriter::isEnabled() const
{
    QMutexLocker locker(&_mutex);
    return _enabled;
}

void LogDiskWriter::setMaxFileSize(qint64 bytes)
{
    QMutexLocker locker(&_mutex);
    _maxFileSize = bytes;
}

void LogDiskWriter::setMaxBackupFiles(int count)
{
    QMutexLocker locker(&_mutex);
    _maxBackupFiles = count;
}

void LogDiskWriter::setMaxPendingEntries(int count)
{
    QMutexLocker locker(&_mutex);
    _maxPendingEntries = count;
}

void LogDiskWriter::write(const QGCLogEntry& entry)
{
    {
        QMutexLocker locker(&_mutex);
        if (!_enabled || _hasError) {
            return;
        }

        _queue.enqueue(entry);

        // Limit queue size - drop oldest entries
        while (_queue.size() > _maxPendingEntries) {
            _queue.dequeue();
        }
    }

    _cv.wakeOne();
}

void LogDiskWriter::flush()
{
    {
        QMutexLocker locker(&_mutex);
        if (!_running) {
            return;
        }
        _flushRequested = true;
    }

    _cv.wakeOne();

    // Wait for flush to complete (with timeout)
    QMutexLocker locker(&_mutex);
    while (_flushRequested && _running) {
        if (!_cv.wait(&_mutex, 500)) {
            break; // Timeout
        }
    }

    _writer->flush();
}

bool LogDiskWriter::hasError() const
{
    QMutexLocker locker(&_mutex);
    return _hasError;
}

void LogDiskWriter::clearError()
{
    {
        QMutexLocker locker(&_mutex);
        if (!_hasError) {
            return;
        }
        _hasError = false;
    }

    _writer->clearError();
}

void LogDiskWriter::_onWriterError(const QString& message)
{
    {
        QMutexLocker locker(&_mutex);
        _hasError = true;
    }
    emit errorOccurred(message);
}

void LogDiskWriter::_startProcessing()
{
    {
        QMutexLocker locker(&_mutex);
        if (_running) {
            return;
        }
        _running = true;
    }

    _future = QtConcurrent::run(&LogDiskWriter::_processingLoop, this);
    qCDebug(LogDiskWriterLog) << "Processing started via QtConcurrent";
}

void LogDiskWriter::_stopProcessing()
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

    qCDebug(LogDiskWriterLog) << "Processing stopped";
}

void LogDiskWriter::_processingLoop()
{
    QQueue<QGCLogEntry> batch;

    while (true) {
        // Wait for entries or timeout
        {
            QMutexLocker locker(&_mutex);

            while (_queue.isEmpty() && !_flushRequested && _running) {
                _cv.wait(&_mutex, kFlushTimeoutMs);
            }

            if (!_running && _queue.isEmpty()) {
                break;
            }

            batch.swap(_queue);
        }

        // Check disk space periodically
        _checkDiskSpace();

        // Check for rotation
        const qint64 currentSize = _writer->fileSize();
        if (currentSize >= 0 && _shouldRotate(currentSize)) {
            _rotateLogs();
        }

        // Format and write entries
        bool hasError;
        {
            QMutexLocker locker(&_mutex);
            hasError = _hasError;
        }

        if (!batch.isEmpty() && !hasError) {
            QString buffer;
            buffer.reserve(batch.size() * 200); // Estimate ~200 chars per entry

            while (!batch.isEmpty()) {
                buffer += batch.dequeue().toString();
                buffer += '\n';
            }

            _writer->write(buffer);
        }
        batch.clear();

        // Handle flush request
        bool flushRequested;
        {
            QMutexLocker locker(&_mutex);
            flushRequested = _flushRequested;
        }

        if (flushRequested) {
            _writer->flush();
            {
                QMutexLocker locker(&_mutex);
                _flushRequested = false;
            }
            _cv.wakeAll();
        }
    }

    // Final flush on shutdown
    {
        QMutexLocker locker(&_mutex);
        batch.swap(_queue);
    }

    if (!batch.isEmpty()) {
        QString buffer;
        while (!batch.isEmpty()) {
            buffer += batch.dequeue().toString();
            buffer += '\n';
        }
        _writer->write(buffer);
    }

    _writer->flush();
}

bool LogDiskWriter::_shouldRotate(qint64 fileSize) const
{
    QMutexLocker locker(&_mutex);
    return fileSize >= _maxFileSize;
}

void LogDiskWriter::_checkDiskSpace()
{
    const QString path = filePath();
    if (path.isEmpty()) {
        return;
    }

    if (!QGCFileHelper::hasSufficientDiskSpace(path, kMinDiskSpaceBytes)) {
        {
            QMutexLocker locker(&_mutex);
            _hasError = true;
        }
        qCWarning(LogDiskWriterLog) << "Insufficient disk space for log file";

        QMetaObject::invokeMethod(this, [this]() {
            emit errorOccurred(QStringLiteral("Insufficient disk space for log file"));
        }, Qt::QueuedConnection);
    }
}

void LogDiskWriter::_rotateLogs()
{
    qCDebug(LogDiskWriterLog) << "Rotating logs";

    // Close current file
    _writer->close();
    _writer->flush(1000);

    const QString path = filePath();
    const QFileInfo fileInfo(path);
    const QString dir = fileInfo.absolutePath();
    const QString name = fileInfo.baseName();
    const QString ext = fileInfo.completeSuffix();

    int maxBackups;
    {
        QMutexLocker locker(&_mutex);
        maxBackups = _maxBackupFiles;
    }

    // Rotate existing backups: name.4.ext → name.5.ext, ...
    for (int i = maxBackups - 1; i >= 1; --i) {
        const QString from = QGCFileHelper::joinPath(dir, QStringLiteral("%1.%2.%3").arg(name).arg(i).arg(ext));
        const QString to = QGCFileHelper::joinPath(dir, QStringLiteral("%1.%2.%3").arg(name).arg(i + 1).arg(ext));

        if (QGCFileHelper::exists(to)) {
            QFile::remove(to);
        }
        if (QGCFileHelper::exists(from)) {
            QGCFileHelper::moveFileOrCopy(from, to);
        }
    }

    // Move current log to .1
    const QString firstBackup = QGCFileHelper::joinPath(dir, QStringLiteral("%1.1.%2").arg(name, ext));
    if (QGCFileHelper::exists(firstBackup)) {
        QFile::remove(firstBackup);
    }
    if (QGCFileHelper::exists(path)) {
        QGCFileHelper::moveFileOrCopy(path, firstBackup);
    }

    qCDebug(LogDiskWriterLog) << "Log rotation complete";
}
