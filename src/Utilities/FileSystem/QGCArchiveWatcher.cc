#include "QGCArchiveWatcher.h"
#include "QGCCompressionJob.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

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
}

QGCArchiveWatcher::~QGCArchiveWatcher()
{
    if (_extractionJob != nullptr && _extractionJob->isRunning()) {
        _extractionJob->cancel();
    }
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

    // Initialize known files for this directory
    if (!_knownFiles.contains(canonicalPath)) {
        QDir dir(canonicalPath);
        const QStringList entries = dir.entryList(QDir::Files);
        QSet<QString> fileSet;
        for (const QString &entry : entries) {
            fileSet.insert(dir.absoluteFilePath(entry));
        }
        _knownFiles[canonicalPath] = fileSet;
        qCDebug(QGCArchiveWatcherLog) << "Initialized" << fileSet.size() << "known files in" << canonicalPath;
    }

    if (_fileWatcher->watchDirectory(canonicalPath)) {
        qCDebug(QGCArchiveWatcherLog) << "Watching directory for archives:" << canonicalPath;
        return true;
    }

    return false;
}

bool QGCArchiveWatcher::unwatchDirectory(const QString &directoryPath)
{
    const QString canonicalPath = QFileInfo(directoryPath).absoluteFilePath();
    _knownFiles.remove(canonicalPath);
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
    _pendingExtractions.clear();

    if (_extractionJob) {
        _extractionJob->cancel();
    }
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
        if (_isWatchedFormat(fullPath)) {
            archives.append(fullPath);
        }
    }

    return archives;
}

void QGCArchiveWatcher::cancelExtraction()
{
    if (_extractionJob != nullptr && _extractionJob->isRunning()) {
        qCDebug(QGCArchiveWatcherLog) << "Cancelling extraction";
        _extractionJob->cancel();
    }
    _pendingExtractions.clear();
}

// ============================================================================
// Private Slots
// ============================================================================

void QGCArchiveWatcher::_onDirectoryChanged(const QString &path)
{
    qCDebug(QGCArchiveWatcherLog) << "Directory changed:" << path;

    QDir dir(path);
    if (!dir.exists()) {
        return;
    }

    // Get current file list
    const QStringList currentEntries = dir.entryList(QDir::Files);
    QSet<QString> currentFiles;
    for (const QString &entry : currentEntries) {
        currentFiles.insert(dir.absoluteFilePath(entry));
    }

    // Find new files
    QSet<QString> &knownFiles = _knownFiles[path];
    const QSet<QString> newFiles = currentFiles - knownFiles;

    // Update known files
    knownFiles = currentFiles;

    // Process new files
    for (const QString &filePath : newFiles) {
        _processNewFile(filePath);
    }
}

void QGCArchiveWatcher::_onExtractionProgress(qreal progress)
{
    _setProgress(progress);
}

void QGCArchiveWatcher::_onExtractionFinished(bool success)
{
    qCDebug(QGCArchiveWatcherLog) << "Extraction finished:" << _currentArchive
                                   << "success:" << success;

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

    _currentArchive.clear();
    _setExtracting(false);
    _setProgress(0.0);

    // Process next pending extraction
    if (!_pendingExtractions.isEmpty()) {
        const QString next = _pendingExtractions.takeFirst();
        _startExtraction(next);
    }
}

// ============================================================================
// Private Methods
// ============================================================================

bool QGCArchiveWatcher::_isWatchedFormat(const QString &filePath) const
{
    const QGCCompression::Format format = QGCCompression::detectFormat(filePath);

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

void QGCArchiveWatcher::_processNewFile(const QString &filePath)
{
    if (!_isWatchedFormat(filePath)) {
        return;
    }

    const QGCCompression::Format format = QGCCompression::detectFormat(filePath);

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
    _setExtracting(true);
    _setProgress(0.0);

    // Create extraction job if needed
    if (_extractionJob == nullptr) {
        _extractionJob = new QGCCompressionJob(this);
        connect(_extractionJob, &QGCCompressionJob::progressChanged,
                this, &QGCArchiveWatcher::_onExtractionProgress);
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
