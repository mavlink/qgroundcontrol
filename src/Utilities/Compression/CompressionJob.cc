#include "CompressionJob.h"

#include <utility>

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QPromise>
#include <QtCore/QRandomGenerator>

#include "Compression.h"
#include "Operation.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(CompressionJobLog, "Utilities.CompressionJob")

CompressionJob::CompressionJob(QObject* context, Callback done)
    : QObject(context)
    , _watcher(new QFutureWatcher<Compression::Outcome>(this))
    , _cancelRequested(std::make_shared<std::atomic_bool>(false))
    , _done(std::move(done))
{
    // Connected before the future is set, as Qt asks
    connect(_watcher, &QFutureWatcher<Compression::Outcome>::progressValueChanged, this,
            &CompressionJob::_onProgressValueChanged);
    connect(_watcher, &QFutureWatcher<Compression::Outcome>::finished, this, &CompressionJob::_onFutureFinished);
}

CompressionJob::~CompressionJob()
{
    if (_ended) {
        return;
    }

    _cancelRequested->store(true);
    _watcher->waitForFinished();
    _removeInput(false);
    _discardOutput(_watcher->result());
}

CompressionJob* CompressionJob::startExtraction(QObject* context, const QString& archivePath,
                                                const QString& outputDirectoryPath, qint64 maxBytes,
                                                Compression::EntryFilter filter, Callback done)
{
    qCDebug(CompressionJobLog) << "Extracting" << archivePath << "to" << outputDirectoryPath;

    auto* const job = new CompressionJob(context, std::move(done));
    job->_start([archivePath, outputDirectoryPath, maxBytes,
                 filter = std::move(filter)](const Compression::ProgressCallback& progress) {
        return Compression::extractArchive(archivePath, outputDirectoryPath,
                                           {.maxBytes = maxBytes, .filter = filter, .progress = progress});
    });
    return job;
}

CompressionJob* CompressionJob::startDecompressionConsumingInput(QObject* context, const QString& inputPath,
                                                                 const QString& outputPath, qint64 maxBytes,
                                                                 Callback done)
{
    auto* const job = new CompressionJob(context, std::move(done));
    job->_inputPath = inputPath;
    job->_outputPath = outputPath;

    // An output path that names the input is refused by Compression::decompressFile(), which can tell that only
    // while the input has its name: such an input is not taken
    if (!Compression::isSameFile(inputPath, outputPath)) {
        // Tried once, under a name that is free: QFile::rename() does not replace a file, and where it cannot rename
        // one it copies it, here on the thread of the caller
        QString takenPath;
        do {
            takenPath = QStringLiteral("%1.qgc-decompressing-%2")
                            .arg(inputPath)
                            .arg(QRandomGenerator::global()->generate(), 8, 16, QLatin1Char('0'));
        } while (QFileInfo::exists(takenPath));
        if (QFile::rename(inputPath, takenPath)) {
            job->_takenInputPath = takenPath;
        } else {
            qCDebug(CompressionJobLog) << "Could not take the input, it is read where it is:" << inputPath;
        }
    }

    qCDebug(CompressionJobLog) << "Decompressing" << inputPath << "as" << job->_takenInputPath << "to" << outputPath;

    job->_start([inputPath, readPath = job->_takenInputPath.isEmpty() ? inputPath : job->_takenInputPath, outputPath,
                 maxBytes](const Compression::ProgressCallback& progress) {
        if (outputPath.isEmpty()) {
            // Compression::decompressFile() would choose a path from the name the input was taken under
            qCDebug(CompressionJobLog) << "No output path to decompress to:" << inputPath;
            return Compression::Outcome{.error = Compression::Error::IoError,
                                        .message = Compression::errorName(Compression::Error::IoError)};
        }
        return Compression::decompressFile(readPath, outputPath, progress, maxBytes);
    });
    return job;
}

void CompressionJob::cancel()
{
    if (_ended) {
        return;
    }

    qCDebug(CompressionJobLog) << "Cancelling the operation";
    _cancelRequested->store(true);
}

void CompressionJob::_onProgressValueChanged(int progressValue)
{
    _setProgress(static_cast<qreal>(progressValue) / 100.0);
}

void CompressionJob::_onFutureFinished()
{
    // The result of the worker is how the operation ended, also when cancel() came too late to stop it
    Compression::Outcome outcome = _watcher->result();

    qCDebug(CompressionJobLog) << "Operation finished:" << outcome.error << outcome.message;

    deleteLater();
    if (outcome && !_cancelRequested->load()) {
        const QPointer<CompressionJob> self(this);
        _setProgress(1.0);
        // A handler of the signal may have deleted the job, whose destructor then ended it
        if (!self) {
            return;
        }
    }

    _ended = true;
    const bool report = !_cancelRequested->load();
    if (!_takenInputPath.isEmpty()) {
        // The outcome names the input as the caller knows it
        outcome.message.replace(_takenInputPath, _inputPath);
        outcome.detail.replace(_takenInputPath, _inputPath);
    }
    _removeInput(report);
    if (!report) {
        _discardOutput(outcome);
        return;
    }

    // Taken out first: the callback may delete the job through its context
    const Callback done = std::exchange(_done, nullptr);
    if (done) {
        done(outcome);
    }
}

void CompressionJob::_start(Work work)
{
    // QtConcurrent passes in the promise of the returned future, through which progress is reported
    _watcher->setFuture(QtConcurrent::run([work = std::move(work),
                                           cancelRequested = std::shared_ptr<const std::atomic_bool>(_cancelRequested)](
                                              QPromise<Compression::Outcome>& promise) {
        using Compression::Error;
        using Compression::Outcome;

        promise.setProgressRange(0, 100);

        const auto progressCallback = [&promise, &cancelRequested](qint64 bytesProcessed, qint64 totalBytes) -> bool {
            if (cancelRequested->load()) {
                return false;
            }

            int progressValue = 0;
            if (totalBytes > 0) {
                progressValue = static_cast<int>((bytesProcessed * 100) / totalBytes);
            } else if (bytesProcessed > 0) {
                // Unknown total: asymptotic progress
                const double normalized = static_cast<double>(bytesProcessed) / 1048576.0;
                progressValue = static_cast<int>(50.0 * (1.0 - (1.0 / (1.0 + normalized))));
            }

            promise.setProgressValue(progressValue);
            return true;
        };

        Outcome outcome{.error = Error::Cancelled, .message = Compression::errorName(Error::Cancelled)};
        try {
            if (!cancelRequested->load()) {
                outcome = work(progressCallback);
            }
        } catch (const std::exception& e) {
            // An exception thrown by an entry filter ends its extraction
            qCDebug(CompressionJobLog) << "Exception during compression operation:" << e.what();
            outcome = {.error = Error::InternalError, .message = QString::fromUtf8(e.what())};
        } catch (...) {
            // Also what is no std::exception: an exception that leaves this function is thrown again where the
            // result is read, on the thread of the job
            qCDebug(CompressionJobLog) << "Exception of an unknown type during compression operation";
            outcome = {.error = Error::InternalError, .message = Compression::errorName(Error::InternalError)};
        }

        if (outcome) {
            promise.setProgressValue(100);
        }
        promise.addResult(outcome);
    }));
}

void CompressionJob::_removeInput(bool reported)
{
    if (!_takenInputPath.isEmpty()) {
        if (!QFile::remove(_takenInputPath)) {
            qCWarning(CompressionJobLog) << "Could not remove the decompressed input:" << _takenInputPath;
        }
    } else if (!_inputPath.isEmpty()) {
        // Once the job ends without calling back, the caller may have written a newer file of that name
        if (reported) {
            (void) QFile::remove(_inputPath);
        } else {
            qCDebug(CompressionJobLog) << "Leaving the input of a cancelled operation:" << _inputPath;
        }
    }
    _inputPath.clear();
    _takenInputPath.clear();
}

void CompressionJob::_discardOutput(const Compression::Outcome& outcome) const
{
    // A failed operation wrote nothing, and a file already at the output path is not its own
    if (outcome && !_outputPath.isEmpty()) {
        qCDebug(CompressionJobLog) << "Removing the output of a cancelled operation:" << _outputPath;
        (void) QFile::remove(_outputPath);
    }
}

void CompressionJob::_setProgress(qreal progress)
{
    if (!qFuzzyCompare(_progress, progress)) {
        _progress = progress;
        emit progressChanged(_progress);
    }
}
