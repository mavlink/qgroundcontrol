#include "QGCCompressionJob.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QMutexLocker>
#include <QtCore/QPointer>
#include <QtCore/QPromise>
#include <algorithm>
#include <exception>

#include "QGCCompression.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCCompressionJobLog, "Utilities.QGCCompressionJob")

// ============================================================================
// Construction / Destruction
// ============================================================================

QGCCompressionJob::QGCCompressionJob(QObject* parent) : QObject(parent), _watcher(new QFutureWatcher<bool>(this))
{
    connect(_watcher, &QFutureWatcher<bool>::progressValueChanged, this, &QGCCompressionJob::_onProgressValueChanged);
    connect(_watcher, &QFutureWatcher<bool>::finished, this, &QGCCompressionJob::_onFutureFinished);
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

QFuture<bool> QGCCompressionJob::extractArchiveAsync(const QString& archivePath, const QString& outputDirectoryPath,
                                                      qint64 maxBytes)
{
    const auto cancelRequested = std::make_shared<std::atomic_bool>(false);
    auto work = [archivePath, outputDirectoryPath, maxBytes](QGCCompression::ProgressCallback progress) {
        return QGCCompression::extractArchive(archivePath, outputDirectoryPath, QGCCompression::Format::Auto, progress,
                                              maxBytes);
    };

    return _runWithProgress(work, cancelRequested);
}

QFuture<bool> QGCCompressionJob::decompressFileAsync(const QString& inputPath, const QString& outputPath,
                                                      qint64 maxBytes)
{
    const auto cancelRequested = std::make_shared<std::atomic_bool>(false);
    auto work = [inputPath, outputPath, maxBytes](QGCCompression::ProgressCallback progress) {
        return QGCCompression::decompressFile(inputPath, outputPath, QGCCompression::Format::Auto, progress, maxBytes);
    };

    return _runWithProgress(work, cancelRequested);
}

// ============================================================================
// Public Slots
// ============================================================================

void QGCCompressionJob::extractArchive(const QString& archivePath, const QString& outputDirectoryPath, qint64 maxBytes)
{
    auto work = [archivePath, outputDirectoryPath, maxBytes](QGCCompression::ProgressCallback progress) {
        return QGCCompression::extractArchive(archivePath, outputDirectoryPath, QGCCompression::Format::Auto, progress,
                                              maxBytes);
    };

    _startOperation(Operation::ExtractArchive, archivePath, outputDirectoryPath, work);
}

void QGCCompressionJob::extractArchiveAtomic(const QString& archivePath, const QString& outputDirectoryPath,
                                              qint64 maxBytes)
{
    auto work = [archivePath, outputDirectoryPath, maxBytes](QGCCompression::ProgressCallback progress) {
        return QGCCompression::extractArchiveAtomic(archivePath, outputDirectoryPath, QGCCompression::Format::Auto,
                                                     progress, maxBytes);
    };

    _startOperation(Operation::ExtractArchiveAtomic, archivePath, outputDirectoryPath, work);
}

void QGCCompressionJob::decompressFile(const QString& inputPath, const QString& outputPath, qint64 maxBytes)
{
    auto work = [inputPath, outputPath, maxBytes](QGCCompression::ProgressCallback progress) {
        return QGCCompression::decompressFile(inputPath, outputPath, QGCCompression::Format::Auto, progress, maxBytes);
    };

    _startOperation(Operation::DecompressFile, inputPath, outputPath, work);
}

void QGCCompressionJob::extractFile(const QString& archivePath, const QString& fileName, const QString& outputPath,
                                    qint64 maxBytes)
{
    auto work = [archivePath, fileName, outputPath, maxBytes](QGCCompression::ProgressCallback progress) {
        return QGCCompression::extractFile(archivePath, fileName, outputPath, QGCCompression::Format::Auto, progress,
                                           maxBytes);
    };

    _startOperation(Operation::ExtractFile, archivePath, outputPath, work);
}

void QGCCompressionJob::extractFiles(const QString& archivePath, const QStringList& fileNames,
                                     const QString& outputDirectoryPath, qint64 maxBytes)
{
    auto work = [archivePath, fileNames, outputDirectoryPath, maxBytes](QGCCompression::ProgressCallback progress) {
        return QGCCompression::extractFiles(archivePath, fileNames, outputDirectoryPath, QGCCompression::Format::Auto,
                                            progress, maxBytes);
    };

    _startOperation(Operation::ExtractFiles, archivePath, outputDirectoryPath, work);
}

void QGCCompressionJob::cancel()
{
    if (!_running) {
        return;
    }

    qCDebug(QGCCompressionJobLog) << "Cancelling operation:" << static_cast<int>(_operation);

    if (_cancelRequested) {
        _cancelRequested->store(true, std::memory_order_release);
    }

    if (_future.isRunning()) {
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
    const Operation completedOperation = _operation;
    const std::shared_ptr<WorkOutcome> completedOutcome = _workOutcome;
    const bool wasCancelled =
        _future.isCanceled() || (_cancelRequested && _cancelRequested->load(std::memory_order_acquire));

    if (wasCancelled) {
        error = QStringLiteral("Operation cancelled");
        qCDebug(QGCCompressionJobLog) << "Operation cancelled:" << static_cast<int>(completedOperation);
    } else {
        try {
            success = _future.result();
            if (!success && completedOutcome) {
                const QMutexLocker locker(&completedOutcome->mutex);
                error = completedOutcome->error;
            }
        } catch (const std::exception &e) {
            error = QString::fromUtf8(e.what());
        }
    }

    qCDebug(QGCCompressionJobLog) << "Operation finished:" << static_cast<int>(completedOperation)
                                  << "success:" << success << "error:" << error;

    _operation = Operation::None;
    _cancelRequested.reset();
    _workOutcome.reset();
    _future = {};
    _completing = true;
    const QPointer<QGCCompressionJob> self(this);

    if (!success && !error.isEmpty()) {
        _setErrorString(error);
    } else if (success) {
        _setErrorString(QString());
    }
    if (!self) {
        return;
    }

    _setProgress(success ? 1.0 : _progress);
    if (!self) {
        return;
    }
    _setRunning(false);
    if (!self) {
        return;
    }
    emit finished(success);
    if (!self) {
        return;
    }
    _completing = false;

    if (_pendingOperation.has_value()) {
        PendingOperation pendingOperation = std::move(_pendingOperation.value());
        _pendingOperation.reset();
        _startOperation(pendingOperation.operation, pendingOperation.source, pendingOperation.output,
                        std::move(pendingOperation.work));
    }
}

// ============================================================================
// Private Methods
// ============================================================================

void QGCCompressionJob::_startOperation(Operation op, const QString& source, const QString& output, WorkFunction work)
{
    if (_completing) {
        if (_pendingOperation.has_value()) {
            qCWarning(QGCCompressionJobLog) << "Operation already queued";
            return;
        }

        _pendingOperation = PendingOperation{
            .operation = op,
            .source = source,
            .output = output,
            .work = std::move(work),
        };
        return;
    }

    if (_running) {
        qCWarning(QGCCompressionJobLog) << "Operation already in progress";
        return;
    }

    qCDebug(QGCCompressionJobLog) << "Starting operation:" << static_cast<int>(op) << "source:" << source
                                  << "output:" << output;

    _operation = op;
    _cancelRequested = std::make_shared<std::atomic_bool>(false);
    _workOutcome = std::make_shared<WorkOutcome>();
    const QPointer<QGCCompressionJob> self(this);
    _setRunning(true);
    if (!self) {
        return;
    }

    // Update paths
    if (_sourcePath != source) {
        _sourcePath = source;
        emit sourcePathChanged(_sourcePath);
        if (!self) {
            return;
        }
    }
    if (_outputPath != output) {
        _outputPath = output;
        emit outputPathChanged(_outputPath);
        if (!self) {
            return;
        }
    }

    _setProgress(0.0);
    if (!self) {
        return;
    }
    _setErrorString(QString());
    if (!self) {
        return;
    }
    // Create and start the future
    _future = _runWithProgress(std::move(work), _cancelRequested, _workOutcome);
    _watcher->setFuture(_future);
}

QFuture<bool> QGCCompressionJob::_runWithProgress(WorkFunction work,
                                                  const std::shared_ptr<std::atomic_bool>& cancelRequested,
                                                  const std::shared_ptr<WorkOutcome>& outcome)
{
    return QtConcurrent::run([work = std::move(work), cancelRequested, outcome](QPromise<bool>& promise) {
        promise.setProgressRange(0, 100);

        // Progress callback that updates QPromise and checks for cancellation
        auto progressCallback = [&promise, cancelRequested](qint64 bytesProcessed, qint64 totalBytes) -> bool {
            if (promise.isCanceled() || (cancelRequested && cancelRequested->load(std::memory_order_acquire))) {
                return false;  // Signal cancellation to the work function
            }

            int progressValue = 0;
            if (totalBytes > 0) {
                progressValue = std::clamp(
                    static_cast<int>((static_cast<double>(bytesProcessed) * 100.0) / static_cast<double>(totalBytes)),
                    0, 100);
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
            if (cancelRequested && cancelRequested->load(std::memory_order_acquire)) {
                success = false;
            } else {
                success = work(progressCallback);
            }
        } catch (const std::exception &e) {
            qCWarning(QGCCompressionJobLog) << "Exception during compression operation:" << e.what();
            if (outcome) {
                const QMutexLocker locker(&outcome->mutex);
                outcome->error = QString::fromUtf8(e.what());
            }
            success = false;
        } catch (...) {
            qCWarning(QGCCompressionJobLog) << "Unknown exception during compression operation";
            if (outcome) {
                const QMutexLocker locker(&outcome->mutex);
                outcome->error = QStringLiteral("Unknown compression error");
            }
            success = false;
        }

        if (!success && outcome) {
            const QMutexLocker locker(&outcome->mutex);
            if (outcome->error.isEmpty()) {
                outcome->error = QGCCompression::lastErrorString();
            }
        }
        if (success) {
        promise.setProgressValue(100);
        }
        promise.addResult(success);
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
