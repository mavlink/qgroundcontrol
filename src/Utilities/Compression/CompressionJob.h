#pragma once

/// @file CompressionJob.h
/// @brief Runs a Compression operation on a worker thread

#include <atomic>
#include <functional>
#include <memory>

#include <QtCore/QFutureWatcher>
#include <QtCore/QObject>

#include "CompressionTypes.h"

/// \brief Runs one Compression operation on a worker thread, and reports its outcome through a callback
///
/// A job is started by startExtraction() or startDecompressionConsumingInput() and is running when they return. It
/// is a child of the context given to it, deletes itself once it has ended, and calls its callback once, on the
/// thread of the context. cancel() on the job, deleting it and deleting the context all end it the same way: the
/// callback is not called. A caller that has moved on needs no check in the callback.
///
/// @code
/// _job = CompressionJob::startExtraction(this, archivePath, _outputDirectory, 0, nullptr,
///                                        [this](const Compression::Outcome& outcome) { ... });
/// connect(_job, &CompressionJob::progressChanged, this, &MyClass::_showProgress);
/// // Later, if the result is no longer wanted. _job is a QPointer: the job deletes itself
/// if (_job) {
///     _job->cancel();
/// }
/// @endcode
class CompressionJob : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(CompressionJob)

public:
    /// Cancels the operation and waits for its worker, which stops at its next progress report: after a block of
    /// data or at an entry, as a rule within milliseconds. A worker that is opening its input, which for a pipe waits
    /// for its writer, or that runs an entry filter, holds the destructor as long. A worker that has not started yet
    /// runs on the thread of the destructor and stops at once
    ~CompressionJob() override;

    /// What a job calls with the outcome of its operation
    using Callback = std::function<void(const Compression::Outcome& outcome)>;

    /// Extract an archive into a new or empty directory, as Compression::extractArchive() does
    /// A failed or cancelled extraction leaves nothing, as Compression::extractArchive() documents. A job that ends
    /// without calling @a done after its worker has completed leaves what it extracted. Extract into a directory of
    /// your own, and remove it when the job ends without calling back.
    /// @param maxBytes Largest size of what is extracted (0 = unlimited)
    /// @param filter Which entries to extract (not set = all of them). It is called on the worker thread
    /// @return The job, already running. Hold it in a QPointer to cancel it
    static CompressionJob* startExtraction(QObject* context, const QString& archivePath,
                                           const QString& outputDirectoryPath, qint64 maxBytes,
                                           Compression::EntryFilter filter, Callback done);

    /// Decompress a file, or copy it if it is not compressed, as Compression::decompressFile() does, and remove it
    /// The job takes the input for itself at once, by renaming it within its directory, so that a file that is
    /// written to @a inputPath meanwhile, such as the next download of that name, is not the job's. It removes the
    /// input once the worker has ended, however the job ends. If the input cannot be renamed, the job reads it where
    /// it is and removes it only when it calls @a done: once it ends without calling back, the caller may have
    /// written a newer file there. A failure message names @a inputPath, not the name the input was taken under.
    ///
    /// A job that ends without calling @a done leaves no output, also if the worker had already written it.
    /// @param outputPath Path of the decompressed file. Without one, or with one that names the input file itself,
    ///        the job fails with Error::IoError
    /// @param maxBytes Largest size the decompressed file may have. Choose it for what is done with the file
    /// @return The job, already running. Hold it in a QPointer to cancel it
    static CompressionJob* startDecompressionConsumingInput(QObject* context, const QString& inputPath,
                                                            const QString& outputPath, qint64 maxBytes, Callback done);

    /// Progress of the operation, from 0.0 to 1.0
    qreal progress() const { return _progress; }

    /// End the job without calling back
    /// The worker stops at its next progress report, and the job deletes itself once it has stopped.
    void cancel();

signals:
    void progressChanged(qreal progress);

private slots:
    void _onProgressValueChanged(int progressValue);
    void _onFutureFinished();

private:
    /// What a job runs on its worker thread. It receives the progress callback, which returns false once the job
    /// is cancelled, and returns the Outcome.
    using Work = std::function<Compression::Outcome(const Compression::ProgressCallback&)>;

    CompressionJob(QObject* context, Callback done);

    void _start(Work work);
    void _setProgress(qreal progress);
    /// Remove the input of a decompression once the worker has ended
    /// @param reported Whether the job calls back, which lets it remove an input it could not take
    void _removeInput(bool reported);
    /// Remove what a successful decompression wrote, for a job that ended without calling back
    void _discardOutput(const Compression::Outcome& outcome) const;

    QFutureWatcher<Compression::Outcome>* _watcher = nullptr;
    /// Set by cancel(). Shared with the worker, so that it outlives a job deleted while the worker winds down
    std::shared_ptr<std::atomic_bool> _cancelRequested;
    Callback _done;
    /// Set once the job has called back or decided not to
    bool _ended = false;
    qreal _progress = 0.0;

    // Of a decompression only
    QString _inputPath;
    /// Name the input was taken under, empty if it could not be renamed
    QString _takenInputPath;
    QString _outputPath;
};
