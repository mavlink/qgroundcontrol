#include "QGCCompressionJob.h"
#include "QGCCompression.h"
#include "QGClibarchive.h"
#include "QGCLoggingCategory.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QPromise>

QGC_LOGGING_CATEGORY(QGCCompressionJobLog, "Utilities.QGCCompressionJob")

// ============================================================================
// Construction / Destruction
// ============================================================================

QGCCompressionJob::QGCCompressionJob(QObject *parent)
    : QObject(parent)
    , _watcher(new QFutureWatcher<bool>(this))
{
    connect(_watcher, &QFutureWatcher<bool>::progressValueChanged,
            this, &QGCCompressionJob::_onProgressValueChanged);
    connect(_watcher, &QFutureWatcher<bool>::finished,
            this, &QGCCompressionJob::_onFutureFinished);
}

QGCCompressionJob::~QGCCompressionJob()
{
    cancel();
    if (_future.isRunning()) {
        _future.waitForFinished();
    }
}

// ============================================================================
// Static Async Methods
// ============================================================================

QFuture<bool> QGCCompressionJob::extractArchiveAsync(const QString &archivePath,
                                                      const QString &outputDirectoryPath,
                                                      qint64 maxBytes)
{
    auto work = [archivePath, outputDirectoryPath, maxBytes](QGCCompression::ProgressCallback progress) {
        return QGCCompression::extractArchive(archivePath, outputDirectoryPath,
                                               QGCCompression::Format::Auto,
                                               progress, maxBytes);
    };

    return _runWithProgress(work);
}

QFuture<bool> QGCCompressionJob::decompressFileAsync(const QString &inputPath,
                                                      const QString &outputPath,
                                                      qint64 maxBytes)
{
    auto work = [inputPath, outputPath, maxBytes](QGCCompression::ProgressCallback progress) {
        return QGCCompression::decompressFile(inputPath, outputPath,
                                               QGCCompression::Format::Auto,
                                               progress, maxBytes);
    };

    return _runWithProgress(work);
}

// ============================================================================
// Public Slots
// ============================================================================

void QGCCompressionJob::extractArchive(const QString &archivePath,
                                        const QString &outputDirectoryPath,
                                        qint64 maxBytes)
{
    auto work = [archivePath, outputDirectoryPath, maxBytes](QGCCompression::ProgressCallback progress) {
        return QGCCompression::extractArchive(archivePath, outputDirectoryPath,
                                               QGCCompression::Format::Auto,
                                               progress, maxBytes);
    };

    _startOperation(Operation::ExtractArchive, archivePath, outputDirectoryPath, work);
}

void QGCCompressionJob::extractArchiveAtomic(const QString &archivePath,
                                              const QString &outputDirectoryPath,
                                              qint64 maxBytes)
{
    auto work = [archivePath, outputDirectoryPath, maxBytes](QGCCompression::ProgressCallback progress) {
        return QGClibarchive::extractArchiveAtomic(archivePath, outputDirectoryPath,
                                                    progress, maxBytes);
    };

    _startOperation(Operation::ExtractArchiveAtomic, archivePath, outputDirectoryPath, work);
}

void QGCCompressionJob::decompressFile(const QString &inputPath,
                                        const QString &outputPath,
                                        qint64 maxBytes)
{
    auto work = [inputPath, outputPath, maxBytes](QGCCompression::ProgressCallback progress) {
        return QGCCompression::decompressFile(inputPath, outputPath,
                                               QGCCompression::Format::Auto,
                                               progress, maxBytes);
    };

    _startOperation(Operation::DecompressFile, inputPath, outputPath, work);
}

void QGCCompressionJob::extractFile(const QString &archivePath,
                                     const QString &fileName,
                                     const QString &outputPath)
{
    auto work = [archivePath, fileName, outputPath](QGCCompression::ProgressCallback) {
        return QGCCompression::extractFile(archivePath, fileName, outputPath);
    };

    _startOperation(Operation::ExtractFile, archivePath, outputPath, work);
}

void QGCCompressionJob::extractFiles(const QString &archivePath,
                                      const QStringList &fileNames,
                                      const QString &outputDirectoryPath)
{
    auto work = [archivePath, fileNames, outputDirectoryPath](QGCCompression::ProgressCallback) {
        return QGCCompression::extractFiles(archivePath, fileNames, outputDirectoryPath);
    };

    _startOperation(Operation::ExtractFiles, archivePath, outputDirectoryPath, work);
}

void QGCCompressionJob::cancel()
{
    if (_running && _future.isRunning()) {
        qCDebug(QGCCompressionJobLog) << "Cancelling operation:" << static_cast<int>(_operation);
        _future.cancel();
    }
}

// ============================================================================
// Private Slots
// ============================================================================

void QGCCompressionJob::_onProgressValueChanged(int progressValue)
{
    _setProgress(static_cast<qreal>(progressValue) / 100.0);
}

void QGCCompressionJob::_onFutureFinished()
{
    bool success = false;
    QString error;

    if (_future.isCanceled()) {
        error = QStringLiteral("Operation cancelled");
        qCDebug(QGCCompressionJobLog) << "Operation cancelled:" << static_cast<int>(_operation);
    } else {
        try {
            success = _future.result();
            if (!success) {
                error = QGCCompression::lastErrorString();
            }
        } catch (const std::exception &e) {
            error = QString::fromUtf8(e.what());
        }
    }

    qCDebug(QGCCompressionJobLog) << "Operation finished:" << static_cast<int>(_operation)
                                   << "success:" << success
                                   << "error:" << error;

    if (!success && !error.isEmpty()) {
        _setErrorString(error);
    } else if (success) {
        _setErrorString(QString());
    }

    _setProgress(success ? 1.0 : _progress);
    _setRunning(false);
    _operation = Operation::None;

    emit finished(success);
}

// ============================================================================
// Private Methods
// ============================================================================

void QGCCompressionJob::_startOperation(Operation op, const QString &source,
                                         const QString &output,
                                         WorkFunction work)
{
    if (_running) {
        qCWarning(QGCCompressionJobLog) << "Operation already in progress";
        return;
    }

    qCDebug(QGCCompressionJobLog) << "Starting operation:" << static_cast<int>(op)
                                   << "source:" << source << "output:" << output;

    _operation = op;

    // Update paths
    if (_sourcePath != source) {
        _sourcePath = source;
        emit sourcePathChanged(_sourcePath);
    }
    if (_outputPath != output) {
        _outputPath = output;
        emit outputPathChanged(_outputPath);
    }

    _setProgress(0.0);
    _setErrorString(QString());
    _setRunning(true);

    // Create and start the future
    _future = _runWithProgress(std::move(work));
    _watcher->setFuture(_future);
}

QFuture<bool> QGCCompressionJob::_runWithProgress(WorkFunction work)
{
    // Use QPromise to create a QFuture with progress reporting
    return QtConcurrent::run([work = std::move(work)]() -> bool {
        QPromise<bool> promise;
        QFuture<bool> future = promise.future();

        promise.start();
        promise.setProgressRange(0, 100);

        // Progress callback that updates QPromise and checks for cancellation
        auto progressCallback = [&promise](qint64 bytesProcessed, qint64 totalBytes) -> bool {
            if (promise.isCanceled()) {
                return false;  // Signal cancellation to the work function
            }

            int progressValue = 0;
            if (totalBytes > 0) {
                progressValue = static_cast<int>((bytesProcessed * 100) / totalBytes);
            } else if (bytesProcessed > 0) {
                // Unknown total - asymptotic progress
                const double normalized = static_cast<double>(bytesProcessed) / 1048576.0;
                progressValue = static_cast<int>(50.0 * (1.0 - (1.0 / (1.0 + normalized))));
            }

            promise.setProgressValue(progressValue);
            return true;  // Continue
        };

        bool success = false;
        try {
            success = work(progressCallback);
        } catch (const std::exception &e) {
            qCWarning(QGCCompressionJobLog) << "Exception during compression operation:" << e.what();
            success = false;
        }

        promise.setProgressValue(100);
        promise.addResult(success);
        promise.finish();

        return success;
    });
}

void QGCCompressionJob::_setProgress(qreal progress)
{
    if (!qFuzzyCompare(_progress, progress)) {
        _progress = progress;
        emit progressChanged(_progress);
    }
}

void QGCCompressionJob::_setRunning(bool running)
{
    if (_running != running) {
        _running = running;
        emit runningChanged(_running);
    }
}

void QGCCompressionJob::_setErrorString(const QString &error)
{
    if (_errorString != error) {
        _errorString = error;
        emit errorStringChanged(_errorString);
    }
}
