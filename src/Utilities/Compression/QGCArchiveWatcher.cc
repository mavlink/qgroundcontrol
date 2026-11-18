#include "QGCArchiveWatcher.h"

#include <algorithm>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

#include "QGCCompressionJob.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCArchiveWatcherLog, "Utilities.QGCArchiveWatcher")

// ============================================================================
// Construction / Destruction
// ============================================================================

QGCArchiveWatcher::QGCArchiveWatcher(QObject *parent)
    : QObject(parent)
    , _fileWatcher(new QGCFileWatcher(this))
{
    _fileWatcher->setDebounceDelay(500);  // Longer debounce for file copy operations

    connect(_fileWatcher, &QGCFileWatcher::directoryChanged,
            this, &QGCArchiveWatcher::_onDirectoryChanged);
    connect(_fileWatcher, &QGCFileWatcher::fileChanged, this, &QGCArchiveWatcher::_onFileChanged);
}

QGCArchiveWatcher::~QGCArchiveWatcher()
{
    _cancelJob();
}

// ============================================================================
// Configuration
// ============================================================================

void QGCArchiveWatcher::setFilterMode(FilterMode mode)
{
    _filterMode = mode;
}

void QGCArchiveWatcher::setAutoDecompress(bool enable)
{
    if (_autoDecompress != enable) {
        _autoDecompress = enable;
        emit autoDecompressChanged(_autoDecompress);
    }
}

void QGCArchiveWatcher::setOutputDirectory(const QString &directory)
{
    if (_outputDirectory != directory) {
        _outputDirectory = directory;
        emit outputDirectoryChanged(_outputDirectory);
    }
}

void QGCArchiveWatcher::setRemoveAfterExtraction(bool remove)
{
    _removeAfterExtraction = remove;
}

void QGCArchiveWatcher::setDebounceDelay(int milliseconds)
{
    _fileWatcher->setDebounceDelay(milliseconds);
}

int QGCArchiveWatcher::debounceDelay() const
{
    return _fileWatcher->debounceDelay();
}

// ============================================================================
// Directory Watching
// ============================================================================

bool QGCArchiveWatcher::watchDirectory(const QString &directoryPath)
{
    if (directoryPath.isEmpty()) {
        qCWarning(QGCArchiveWatcherLog) << "watchDirectory: empty path";
        return false;
    }

    const QString canonicalPath = QFileInfo(directoryPath).absoluteFilePath();

    if (!QFileInfo(canonicalPath).isDir()) {
        qCWarning(QGCArchiveWatcherLog) << "watchDirectory: not a directory:" << directoryPath;
        return false;
    }

    // What is known of a directory is that of its watch. A directory that is not being watched starts from the
    // files that are in it now, whatever is left of a watch that has ended and has not been heard of yet.
    if (!_fileWatcher->isWatchingDirectory(canonicalPath)) {
        _forgetDirectory(canonicalPath);
        const QSet<QString> fileSet = _filesIn(canonicalPath);
        _knownFiles[canonicalPath] = fileSet;
        qCDebug(QGCArchiveWatcherLog) << "Initialized" << fileSet.size() << "known files in" << canonicalPath;
    }

    if (!_fileWatcher->watchDirectory(canonicalPath, nullptr)) {
        _forgetDirectory(canonicalPath);
        return false;
    }
    qCDebug(QGCArchiveWatcherLog) << "Watching directory for archives:" << canonicalPath;

    // The watch does not tell of a file that landed after the known files were listed and before it was in
    // place. If one did, the directory is looked at when a change that happened now would be reported.
    if (!(_filesIn(canonicalPath) - _knownFiles.value(canonicalPath)).isEmpty()) {
        QTimer::singleShot(_fileWatcher->debounceDelay(), this,
                           [this, canonicalPath]() { _onDirectoryChanged(canonicalPath); });
    }

    return true;
}

bool QGCArchiveWatcher::unwatchDirectory(const QString &directoryPath)
{
    const QString canonicalPath = QFileInfo(directoryPath).absoluteFilePath();
    _forgetDirectory(canonicalPath);
    return _fileWatcher->unwatchDirectory(canonicalPath);
}

QStringList QGCArchiveWatcher::watchedDirectories() const
{
    return _fileWatcher->watchedDirectories();
}

void QGCArchiveWatcher::clear()
{
    _fileWatcher->clear();
    _knownFiles.clear();
    _filesWithoutContent.clear();
    _pendingExtractions.clear();
    _currentArchive.clear();
    _setExtracting(false);
    _setProgress(0.0);

    _cancelJob();
}

// ============================================================================
// Manual Operations
// ============================================================================

QStringList QGCArchiveWatcher::scanDirectory(const QString &directoryPath) const
{
    QStringList archives;

    QDir dir(directoryPath);
    if (!dir.exists()) {
        return archives;
    }

    const QStringList entries = dir.entryList(QDir::Files);
    for (const QString &entry : entries) {
        const QString fullPath = dir.absoluteFilePath(entry);
        if (_isWatchedFormat(QGCCompression::detectFormat(fullPath))) {
            archives.append(fullPath);
        }
    }

    return archives;
}

void QGCArchiveWatcher::cancelExtraction()
{
    _cancelJob(true);
    _pendingExtractions.clear();
    _currentArchive.clear();
    _setExtracting(false);
    _setProgress(0.0);
}

// ============================================================================
// Private Slots
// ============================================================================

void QGCArchiveWatcher::_onDirectoryChanged(const QString &path)
{
    qCDebug(QGCArchiveWatcherLog) << "Directory changed:" << path;

    if (!_fileWatcher->isWatchingDirectory(path)) {
        // The watch ended with the directory, and what was known of the directory goes with the watch
        _forgetDirectory(path);
        return;
    }

    if (!QDir(path).exists()) {
        return;
    }

    const QSet<QString> currentFiles = _filesIn(path);

    // Find new files
    QSet<QString> &knownFiles = _knownFiles[path];
    const QSet<QString> newFiles = currentFiles - knownFiles;

    // Update known files
    knownFiles = currentFiles;

    // Process new files
    for (const QString &filePath : newFiles) {
        _processNewFile(path, filePath);
    }
}

void QGCArchiveWatcher::_onFileChanged(const QString& path)
{
    const auto waiting = std::ranges::find_if(_filesWithoutContent,
                                              [&path](const QSet<QString>& files) { return files.contains(path); });
    if (waiting == _filesWithoutContent.end()) {
        return;
    }
    const QString directoryPath = waiting.key();
    waiting->remove(path);
    (void) _fileWatcher->unwatchFile(path);

    // Judged as when it appeared, which watches it again if it is still without content
    if (QFileInfo::exists(path)) {
        _processNewFile(directoryPath, path);
    }
}

void QGCArchiveWatcher::_onExtractionFinished(bool success)
{
    // The result of an extraction that was cancelled is not reported, and nothing is started after it
    const bool cancelled = _cancelPending;
    _cancelPending = false;

    if (!cancelled) {
        qCDebug(QGCArchiveWatcherLog) << "Extraction finished:" << _currentArchive << "success:" << success;

        QString outputPath = _extractionJob->outputPath();
        QString errorString = success ? QString() : _extractionJob->errorString();

        // Remove source archive if configured and extraction succeeded
        if (success && _removeAfterExtraction) {
            if (QFile::remove(_currentArchive)) {
                qCDebug(QGCArchiveWatcherLog) << "Removed source archive:" << _currentArchive;
            } else {
                qCWarning(QGCArchiveWatcherLog) << "Failed to remove source archive:" << _currentArchive;
            }
        }

        emit extractionComplete(_currentArchive, outputPath, success, errorString);
    }

    _currentArchive.clear();
    _setExtracting(false);
    _setProgress(0.0);

    // Process next pending extraction
    if (!cancelled && !_pendingExtractions.isEmpty()) {
        const QString next = _pendingExtractions.takeFirst();
        _startExtraction(next);
    }
}

// ============================================================================
// Private Methods
// ============================================================================

QSet<QString> QGCArchiveWatcher::_filesIn(const QString& directoryPath)
{
    const QDir dir(directoryPath);
    const QStringList entries = dir.entryList(QDir::Files);
    QSet<QString> files;
    for (const QString& entry : entries) {
        files.insert(dir.absoluteFilePath(entry));
    }
    return files;
}

void QGCArchiveWatcher::_forgetDirectory(const QString& directoryPath)
{
    _knownFiles.remove(directoryPath);
    const QSet<QString> waitingFiles = _filesWithoutContent.take(directoryPath);
    for (const QString& filePath : waitingFiles) {
        (void) _fileWatcher->unwatchFile(filePath);
    }
}

bool QGCArchiveWatcher::_isWatchedFormat(QGCCompression::Format format) const
{
    switch (_filterMode) {
    case FilterMode::Archives:
        return QGCCompression::isArchiveFormat(format);
    case FilterMode::Compressed:
        return QGCCompression::isCompressionFormat(format);
    case FilterMode::Both:
        return QGCCompression::isArchiveFormat(format) ||
               QGCCompression::isCompressionFormat(format);
    }

    return false;
}

void QGCArchiveWatcher::_processNewFile(const QString& directoryPath, const QString& filePath)
{
    const QGCCompression::Format format = QGCCompression::detectFormat(filePath);
    if (!_isWatchedFormat(format)) {
        // The content tells the format of a file whose name does not, and a file that was just created can be
        // without any. The directory does not change when the file is written, so the file is watched.
        const QFileInfo fileInfo(filePath);
        if ((format == QGCCompression::Format::Auto) && fileInfo.exists() && (fileInfo.size() == 0) &&
            _fileWatcher->watchFile(filePath, nullptr)) {
            _filesWithoutContent[directoryPath].insert(filePath);
        }
        return;
    }

    qCDebug(QGCArchiveWatcherLog) << "Detected archive:" << filePath
                                   << "format:" << QGCCompression::formatName(format);

    emit archiveDetected(filePath, format);

    if (_autoDecompress) {
        if (_extracting) {
            // Queue for later
            if (!_pendingExtractions.contains(filePath)) {
                _pendingExtractions.append(filePath);
                qCDebug(QGCArchiveWatcherLog) << "Queued for extraction:" << filePath;
            }
        } else {
            _startExtraction(filePath);
        }
    }
}

void QGCArchiveWatcher::_startExtraction(const QString &archivePath)
{
    if (!QFileInfo::exists(archivePath)) {
        qCWarning(QGCArchiveWatcherLog) << "Archive no longer exists:" << archivePath;
        return;
    }

    // Determine output path
    QString outputPath = _outputDirectory;
    if (outputPath.isEmpty()) {
        outputPath = QFileInfo(archivePath).absolutePath();
    }

    // For single-file compression, use decompressFile
    // For archives, use extractArchive
    const QGCCompression::Format format = QGCCompression::detectFormat(archivePath);
    const bool isArchive = QGCCompression::isArchiveFormat(format);

    qCDebug(QGCArchiveWatcherLog) << "Starting extraction:" << archivePath
                                   << "to" << outputPath
                                   << (isArchive ? "(archive)" : "(compressed file)");

    _currentArchive = archivePath;
    _cancelPending = false;
    _setExtracting(true);
    _setProgress(0.0);

    // Create extraction job if needed
    if (_extractionJob == nullptr) {
        _extractionJob = new QGCCompressionJob(this);
        connect(_extractionJob, &QGCCompressionJob::progressChanged, this, &QGCArchiveWatcher::_setProgress);
        connect(_extractionJob, &QGCCompressionJob::finished,
                this, &QGCArchiveWatcher::_onExtractionFinished);
    }

    if (isArchive) {
        _extractionJob->extractArchive(archivePath, outputPath);
    } else {
        // For compressed files, output is a file, not directory
        const QString strippedName = QFileInfo(QGCCompression::strippedPath(archivePath)).fileName();
        const QString decompressedPath = outputPath + "/" + strippedName;
        _extractionJob->decompressFile(archivePath, decompressedPath);
    }
}

void QGCArchiveWatcher::_cancelJob(bool announce)
{
    if (_extractionJob == nullptr || !_extractionJob->isRunning()) {
        return;
    }
    if (announce) {
        qCDebug(QGCArchiveWatcherLog) << "Cancelling extraction";
    }
    _cancelPending = true;
    _extractionJob->cancel();
}

void QGCArchiveWatcher::_setExtracting(bool extracting)
{
    if (_extracting != extracting) {
        _extracting = extracting;
        emit extractingChanged(_extracting);
    }
}

void QGCArchiveWatcher::_setProgress(qreal progress)
{
    if (!qFuzzyCompare(_progress, progress)) {
        _progress = progress;
        emit progressChanged(_progress);
    }
}
