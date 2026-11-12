#include "QGCCompressionJob.h"
#include "QGCCompression.h"
#include "QGCLoggingCategory.h"

#include <QtConcurrent/QtConcurrent>
#include <QtCore/QPromise>

QGC_LOGGING_CATEGORY(QGCCompressionJobLog, "Utilities.QGCCompressionJob")

// ============================================================================
// Construction / Destruction
// ============================================================================

QGCCompressionJob::QGCCompressionJob(QObject* parent)
    : QObject(parent)
    , _watcher(new QFutureWatcher<QGCCompression::Outcome>(this))
{
    connect(_watcher, &QFutureWatcher<QGCCompression::Outcome>::progressValueChanged, this,
            &QGCCompressionJob::_onProgressValueChanged);
    connect(_watcher, &QFutureWatcher<QGCCompression::Outcome>::finished, this, &QGCCompressionJob::_onFutureFinished);
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

QFuture<QGCCompression::Outcome> QGCCompressionJob::extractArchiveAsync(const QString& archivePath,
                                                                        const QString& outputDirectoryPath,
                                                                        qint64 maxBytes)
{
    return run([archivePath, outputDirectoryPath, maxBytes](QGCCompression::ProgressCallback progress) {
        return QGCCompression::extractArchive(archivePath, outputDirectoryPath, QGCCompression::Format::Auto, progress,
                                              maxBytes);
    });
}

QFuture<QGCCompression::Outcome> QGCCompressionJob::decompressFileAsync(const QString& inputPath,
                                                                        const QString& outputPath, qint64 maxBytes)
{
    return run([inputPath, outputPath, maxBytes](QGCCompression::ProgressCallback progress) {
        return QGCCompression::decompressFile(inputPath, outputPath, QGCCompression::Format::Auto, progress, maxBytes);
    });
}

// ============================================================================
// Public Slots
// ============================================================================

void QGCCompressionJob::start(Work work, const QString& source, const QString& output)
{
    _startOperation(Operation::Custom, source, output, std::move(work));
}

void QGCCompressionJob::extractArchive(const QString& archivePath, const QString& outputDirectoryPath, qint64 maxBytes)
{
    _startOperation(Operation::ExtractArchive, archivePath, outputDirectoryPath,
                    [archivePath, outputDirectoryPath, maxBytes](QGCCompression::ProgressCallback progress) {
                        return QGCCompression::extractArchive(archivePath, outputDirectoryPath,
                                                              QGCCompression::Format::Auto, progress, maxBytes);
                    });
}

void QGCCompressionJob::extractArchiveAtomic(const QString& archivePath, const QString& outputDirectoryPath,
                                             qint64 maxBytes)
{
    _startOperation(Operation::ExtractArchiveAtomic, archivePath, outputDirectoryPath,
                    [archivePath, outputDirectoryPath, maxBytes](QGCCompression::ProgressCallback progress) {
                        return QGCCompression::extractArchiveAtomic(archivePath, outputDirectoryPath,
                                                                    QGCCompression::Format::Auto, progress, maxBytes);
                    });
}

void QGCCompressionJob::decompressFile(const QString& inputPath, const QString& outputPath, qint64 maxBytes,
                                       QGCCompression::PlainInput plainInput)
{
    _startOperation(Operation::DecompressFile, inputPath, outputPath,
                    [inputPath, outputPath, maxBytes, plainInput](QGCCompression::ProgressCallback progress) {
                        return QGCCompression::decompressFile(inputPath, outputPath, QGCCompression::Format::Auto,
                                                              progress, maxBytes, plainInput);
                    });
}

void QGCCompressionJob::extractFile(const QString& archivePath, const QString& fileName, const QString& outputPath)
{
    _startOperation(Operation::ExtractFile, archivePath, outputPath,
                    [archivePath, fileName, outputPath](QGCCompression::ProgressCallback) {
                        return QGCCompression::extractFile(archivePath, fileName, outputPath);
                    });
}

void QGCCompressionJob::extractFiles(const QString& archivePath, const QStringList& fileNames,
                                     const QString& outputDirectoryPath)
{
    _startOperation(Operation::ExtractFiles, archivePath, outputDirectoryPath,
                    [archivePath, fileNames, outputDirectoryPath](QGCCompression::ProgressCallback) {
                        return QGCCompression::extractFiles(archivePath, fileNames, outputDirectoryPath);
                    });
}

void QGCCompressionJob::cancel()
{
    if (!_running || !_future.isRunning()) {
        return;
    }

    qCDebug(QGCCompressionJobLog) << "Cancelling operation:" << static_cast<int>(_operation);

    _future.cancel();
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
            const QGCCompression::Outcome outcome = _future.result();
            success = static_cast<bool>(outcome);
            error = outcome.message;
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

void QGCCompressionJob::_startOperation(Operation op, const QString& source, const QString& output, Work work)
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
    _future = run(std::move(work));
    _watcher->setFuture(_future);
}

QFuture<QGCCompression::Outcome> QGCCompressionJob::run(Work work)
{
    using QGCCompression::Error;
    using QGCCompression::Outcome;

    // QtConcurrent passes in the promise of the returned future, so progress and cancellation reach the caller
    return QtConcurrent::run([work = std::move(work)](QPromise<Outcome>& promise) {
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

        Outcome outcome{.error = Error::Cancelled, .message = QGCCompression::errorName(Error::Cancelled)};
        try {
            if (!promise.isCanceled()) {
                outcome = work(progressCallback);
            }
        } catch (const std::exception &e) {
            qCWarning(QGCCompressionJobLog) << "Exception during compression operation:" << e.what();
            outcome = {.error = Error::InternalError, .message = QString::fromUtf8(e.what())};
        }

        if (outcome) {
            promise.setProgressValue(100);
        }
        promise.addResult(outcome);
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
