#pragma once

/// @file QGCCompressionJob.h
/// @brief QObject wrapper for async compression operations using QtConcurrent/QPromise

#include <functional>

#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <QtCore/QObject>

#include "QGCCompression.h"

/// \brief QObject wrapper for compression operations with progress signals
///
/// Uses QtConcurrent and QPromise for modern async operations
/// Can be used from C++ or QML
///
/// Example C++ usage:
/// @code
/// auto *job = new QGCCompressionJob(this);
/// connect(job, &QGCCompressionJob::finished, this, [](bool success) {
///     qDebug() << "Extraction" << (success ? "succeeded" : "failed");
/// });
/// connect(job, &QGCCompressionJob::progressChanged, this, [](qreal p) {
///     qDebug() << "Progress:" << (p * 100) << "%";
/// });
/// job->extractArchive("archive.zip", "/output/dir");
/// @endcode
///
/// Example QML usage:
/// @code
/// QGCCompressionJob {
///     id: extractionJob
///     onProgressChanged: progressBar.value = progress
///     onFinished: if (!success) console.log("Error:", errorString)
/// }
/// Button {
///     text: extractionJob.running ? "Cancel" : "Extract"
///     onClicked: extractionJob.running ? extractionJob.cancel()
///                                       : extractionJob.extractArchive(path, outputDir)
/// }
/// @endcode
///
/// Alternative: Use static async methods that return QFuture directly:
/// @code
/// using Outcome = QGCCompression::Outcome;
/// QFuture<Outcome> future = QGCCompressionJob::extractArchiveAsync("archive.zip", "/output");
/// QFutureWatcher<Outcome> *watcher = new QFutureWatcher<Outcome>(this);
/// connect(watcher, &QFutureWatcher<Outcome>::progressValueChanged, this, [](int value) {
///     qDebug() << "Progress:" << value << "%";
/// });
/// connect(watcher, &QFutureWatcher<Outcome>::finished, this, [watcher]() {
///     qDebug() << "Done:" << watcher->result().message;
///     watcher->deleteLater();
/// });
/// watcher->setFuture(future);
/// @endcode
///
class QGCCompressionJob : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QGCCompressionJob)

    /// Current progress (0.0 to 1.0)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged FINAL)

    /// Whether an operation is currently running
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged FINAL)

    /// Error string from last failed operation (empty if last operation succeeded)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged FINAL)

    /// Source path for current/last operation
    Q_PROPERTY(QString sourcePath READ sourcePath NOTIFY sourcePathChanged FINAL)

    /// Output path for current/last operation
    Q_PROPERTY(QString outputPath READ outputPath NOTIFY outputPathChanged FINAL)

public:
    /// Operation type
    enum class Operation {
        None,
        ExtractArchive,
        ExtractArchiveAtomic,
        DecompressFile,
        ExtractFile,
        ExtractFiles,
        Custom  ///< Work that was given to start()
    };
    Q_ENUM(Operation)

    /// What a job runs on its worker thread. It is given the callback to report progress to, which returns false
    /// once the job is cancelled, and returns how it ended.
    using Work = std::function<QGCCompression::Outcome(QGCCompression::ProgressCallback)>;

    explicit QGCCompressionJob(QObject *parent = nullptr);
    ~QGCCompressionJob() override;

    // Property getters
    qreal progress() const { return _progress; }
    bool isRunning() const { return _running; }
    QString errorString() const { return _errorString; }
    QString sourcePath() const { return _sourcePath; }
    QString outputPath() const { return _outputPath; }

    /// Get current operation type
    Operation currentOperation() const { return _operation; }

    /// Get the QFuture for the current operation (for advanced use)
    /// Returns invalid future if no operation is running
    QFuture<QGCCompression::Outcome> future() const { return _future; }

    /// Run @a work as the operation of this job, which is what each slot does with one function of QGCCompression
    /// Does nothing, after a warning, while an operation is running.
    /// @param source What sourcePath() tells of the operation
    /// @param output What outputPath() tells of the operation
    void start(Work work, const QString& source = {}, const QString& output = {});

    // ========================================================================
    // Static Async Methods (return QFuture directly)
    // ========================================================================

    /// Run @a work on a worker thread, which is what the two functions below do with one function of QGCCompression
    /// @return Future of the outcome, with progress reporting (0-100). Cancelling it cancels the work
    static QFuture<QGCCompression::Outcome> run(Work work);

    /// Extract archive asynchronously, returns QFuture with progress support
    /// @param archivePath Path to archive file
    /// @param outputDirectoryPath Directory to extract to
    /// @param maxBytes Maximum decompressed size (0 = unlimited)
    /// @return Future of the outcome, with progress reporting (0-100)
    static QFuture<QGCCompression::Outcome> extractArchiveAsync(const QString& archivePath,
                                                                const QString& outputDirectoryPath,
                                                                qint64 maxBytes = 0);

    /// Decompress file asynchronously, returns QFuture with progress support
    /// @param inputPath Path to compressed file
    /// @param outputPath Path for decompressed output (empty = auto-detect)
    /// @param maxBytes Maximum decompressed size (0 = unlimited)
    /// @return Future of the outcome, with progress reporting (0-100)
    static QFuture<QGCCompression::Outcome> decompressFileAsync(
        const QString& inputPath, const QString& outputPath = QString(),
        qint64 maxBytes = QGCCompression::kDefaultMaxDecompressedFileBytes);

public slots:
    /// Extract archive to directory (non-atomic, faster)
    /// @param archivePath Path to archive file
    /// @param outputDirectoryPath Directory to extract to
    /// @param maxBytes Maximum decompressed size (0 = unlimited)
    void extractArchive(const QString &archivePath, const QString &outputDirectoryPath,
                        qint64 maxBytes = 0);

    /// Extract archive atomically (all-or-nothing, safer)
    /// @param archivePath Path to archive file
    /// @param outputDirectoryPath Directory to extract to
    /// @param maxBytes Maximum decompressed size (0 = unlimited)
    void extractArchiveAtomic(const QString &archivePath, const QString &outputDirectoryPath,
                              qint64 maxBytes = 0);

    /// Decompress single compressed file (.gz, .xz, etc.)
    /// @param inputPath Path to compressed file
    /// @param outputPath Path for decompressed output (empty = auto-detect)
    /// @param maxBytes Maximum size of the output (0 = unlimited)
    /// @param plainInput Whether a file that only has the name of a compressed one fails the job or is copied
    ///        to the output as it is: see QGCCompression::decompressFile()
    void decompressFile(const QString& inputPath, const QString& outputPath = QString(),
                        qint64 maxBytes = QGCCompression::kDefaultMaxDecompressedFileBytes,
                        QGCCompression::PlainInput plainInput = QGCCompression::PlainInput::Refuse);

    /// Extract single file from archive
    /// @param archivePath Path to archive file
    /// @param fileName Name of file inside archive
    /// @param outputPath Output file path
    void extractFile(const QString &archivePath, const QString &fileName,
                     const QString &outputPath);

    /// Extract multiple files from archive
    /// @param archivePath Path to archive file
    /// @param fileNames Names of files inside archive
    /// @param outputDirectoryPath Output directory
    void extractFiles(const QString &archivePath, const QStringList &fileNames,
                      const QString &outputDirectoryPath);

    /// Cancel current operation
    void cancel();

signals:
    /// Emitted when progress changes (0.0 to 1.0)
    void progressChanged(qreal progress);

    /// Emitted when running state changes
    void runningChanged(bool running);

    /// Emitted when operation completes
    /// @param success true if operation succeeded
    void finished(bool success);

    /// Emitted when error string changes
    void errorStringChanged(const QString &errorString);

    /// Emitted when source path changes
    void sourcePathChanged(const QString &sourcePath);

    /// Emitted when output path changes
    void outputPathChanged(const QString &outputPath);

private slots:
    void _onProgressValueChanged(int progressValue);
    void _onFutureFinished();

private:
    void _startOperation(Operation op, const QString& source, const QString& output, Work work);
    void _setProgress(qreal progress);
    void _setRunning(bool running);
    void _setErrorString(const QString &error);

    QFutureWatcher<QGCCompression::Outcome>* _watcher = nullptr;
    QFuture<QGCCompression::Outcome> _future;

    qreal _progress = 0.0;
    bool _running = false;
    QString _errorString;
    QString _sourcePath;
    QString _outputPath;
    Operation _operation = Operation::None;
};
