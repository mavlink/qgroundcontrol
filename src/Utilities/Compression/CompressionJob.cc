#include "CompressionJob.h"

#include <utility>

#include <QtConcurrent/QtConcurrentRun>
#include <QtCore/QPromise>
#include <QtCore/QThreadPool>

#include "Compression.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(CompressionJobLog, "Utilities.CompressionJob")

namespace {

/// The pool of CompressionJob. Two threads: an operation is bound by the disk, so more would not make the jobs
/// faster, and a second one lets a short decompression run beside a long extraction
class CompressionJobThreadPool : public QThreadPool
{
public:
    CompressionJobThreadPool()
    {
        setMaxThreadCount(2);
        setObjectName(QStringLiteral("CompressionJob"));
    }
};

Q_GLOBAL_STATIC(CompressionJobThreadPool, compressionJobThreadPool)

}  // namespace

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
}

CompressionJob* CompressionJob::startExtraction(QObject* context, const QString& archivePath,
                                                const QString& outputDirectoryPath, Compression::ExtractOptions options,
                                                Callback done)
{
    qCDebug(CompressionJobLog) << "Extracting" << archivePath << "to" << outputDirectoryPath;

    auto* const job = new CompressionJob(context, std::move(done));
    job->_start([archivePath, outputDirectoryPath,
                 options = std::move(options)](const Compression::ProgressCallback& progress) {
        Compression::ExtractOptions jobOptions = options;
        jobOptions.progress = [&progress, &callerProgress = options.progress](qint64 bytesProcessed,
                                                                              qint64 totalBytes) {
            return progress(bytesProcessed, totalBytes) &&
                   (!callerProgress || callerProgress(bytesProcessed, totalBytes));
        };
        return Compression::extractArchive(archivePath, outputDirectoryPath, jobOptions);
    });
    return job;
}

CompressionJob* CompressionJob::startDecompression(QObject* context, const QString& inputPath,
                                                   const QString& outputPath, qint64 maxBytes, Callback done)
{
    qCDebug(CompressionJobLog) << "Decompressing" << inputPath << "to" << outputPath;

    auto* const job = new CompressionJob(context, std::move(done));
    job->_start([inputPath, outputPath, maxBytes](const Compression::ProgressCallback& progress) {
        return Compression::decompressFileToFile(inputPath, outputPath, progress, maxBytes);
    });
    return job;
}

QThreadPool* CompressionJob::threadPool()
{
    return compressionJobThreadPool();
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
    const Compression::Outcome outcome = _watcher->result();

    qCDebug(CompressionJobLog) << "Operation finished:" << outcome.error << outcome.message;

    deleteLater();

    // An operation that succeeded has reported progress 1.0 by now: the worker sets the value that ends the range,
    // which a future always passes on, before its result
    _ended = true;
    if (_cancelRequested->load()) {
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
    auto run = [work = std::move(work), cancelRequested = std::shared_ptr<const std::atomic_bool>(_cancelRequested)](
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
            outcome = {.error = Error::InternalError,
                       .message = Compression::errorName(Error::InternalError),
                       .detail = QString::fromUtf8(e.what())};
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
    };
    _watcher->setFuture(QtConcurrent::run(threadPool(), std::move(run)));
}

void CompressionJob::_setProgress(qreal progress)
{
    if (!qFuzzyCompare(_progress, progress)) {
        _progress = progress;
        emit progressChanged(_progress);
    }
}
