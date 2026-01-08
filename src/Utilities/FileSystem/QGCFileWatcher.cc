#include "QGCFileWatcher.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

QGC_LOGGING_CATEGORY(QGCFileWatcherLog, "Utilities.QGCFileWatcher")

// ============================================================================
// Construction / Destruction
// ============================================================================

QGCFileWatcher::QGCFileWatcher(QObject *parent)
    : QObject(parent)
    , _watcher(new QFileSystemWatcher(this))
    , _debounceTimer(new QTimer(this))
{
    connect(_watcher, &QFileSystemWatcher::fileChanged,
            this, &QGCFileWatcher::_onFileChanged);
    connect(_watcher, &QFileSystemWatcher::directoryChanged,
            this, &QGCFileWatcher::_onDirectoryChanged);

    _debounceTimer->setSingleShot(true);
    connect(_debounceTimer, &QTimer::timeout,
            this, &QGCFileWatcher::_processPendingChanges);
}

QGCFileWatcher::~QGCFileWatcher()
{
    clear();
}

// ============================================================================
// Configuration
// ============================================================================

void QGCFileWatcher::setDebounceDelay(int milliseconds)
{
    _debounceDelay = qMax(0, milliseconds);
}

// ============================================================================
// File Watching
// ============================================================================

bool QGCFileWatcher::watchFile(const QString &filePath, ChangeCallback callback)
{
    if (filePath.isEmpty()) {
        qCWarning(QGCFileWatcherLog) << "watchFile: empty path";
        return false;
    }

    const QString canonicalPath = QFileInfo(filePath).absoluteFilePath();

    if (!QFileInfo::exists(canonicalPath)) {
        qCWarning(QGCFileWatcherLog) << "watchFile: file does not exist:" << filePath;
        return false;
    }

    if (_watcher->addPath(canonicalPath)) {
        if (callback) {
            _fileCallbacks[canonicalPath] = callback;
        }
        qCDebug(QGCFileWatcherLog) << "Watching file:" << canonicalPath;
        return true;
    }

    // Already watching
    if (_watcher->files().contains(canonicalPath)) {
        if (callback) {
            _fileCallbacks[canonicalPath] = callback;
        }
        return true;
    }

    qCWarning(QGCFileWatcherLog) << "watchFile: failed to add:" << filePath;
    return false;
}

bool QGCFileWatcher::watchFile(const QString &filePath)
{
    return watchFile(filePath, nullptr);
}

bool QGCFileWatcher::unwatchFile(const QString &filePath)
{
    const QString canonicalPath = QFileInfo(filePath).absoluteFilePath();

    _fileCallbacks.remove(canonicalPath);
    _persistentFiles.remove(canonicalPath);
    _pendingFileChanges.remove(canonicalPath);

    if (_watcher->removePath(canonicalPath)) {
        qCDebug(QGCFileWatcherLog) << "Stopped watching file:" << canonicalPath;
        return true;
    }

    return false;
}

bool QGCFileWatcher::isWatchingFile(const QString &filePath) const
{
    const QString canonicalPath = QFileInfo(filePath).absoluteFilePath();
    return _watcher->files().contains(canonicalPath);
}

QStringList QGCFileWatcher::watchedFiles() const
{
    return _watcher->files();
}

// ============================================================================
// Directory Watching
// ============================================================================

bool QGCFileWatcher::watchDirectory(const QString &directoryPath, ChangeCallback callback)
{
    if (directoryPath.isEmpty()) {
        qCWarning(QGCFileWatcherLog) << "watchDirectory: empty path";
        return false;
    }

    const QString canonicalPath = QFileInfo(directoryPath).absoluteFilePath();

    if (!QFileInfo(canonicalPath).isDir()) {
        qCWarning(QGCFileWatcherLog) << "watchDirectory: not a directory:" << directoryPath;
        return false;
    }

    if (_watcher->addPath(canonicalPath)) {
        if (callback) {
            _directoryCallbacks[canonicalPath] = callback;
        }
        qCDebug(QGCFileWatcherLog) << "Watching directory:" << canonicalPath;
        return true;
    }

    // Already watching
    if (_watcher->directories().contains(canonicalPath)) {
        if (callback) {
            _directoryCallbacks[canonicalPath] = callback;
        }
        return true;
    }

    qCWarning(QGCFileWatcherLog) << "watchDirectory: failed to add:" << directoryPath;
    return false;
}

bool QGCFileWatcher::watchDirectory(const QString &directoryPath)
{
    return watchDirectory(directoryPath, nullptr);
}

bool QGCFileWatcher::unwatchDirectory(const QString &directoryPath)
{
    const QString canonicalPath = QFileInfo(directoryPath).absoluteFilePath();

    _directoryCallbacks.remove(canonicalPath);
    _pendingDirectoryChanges.remove(canonicalPath);

    if (_watcher->removePath(canonicalPath)) {
        qCDebug(QGCFileWatcherLog) << "Stopped watching directory:" << canonicalPath;
        return true;
    }

    return false;
}

bool QGCFileWatcher::isWatchingDirectory(const QString &directoryPath) const
{
    const QString canonicalPath = QFileInfo(directoryPath).absoluteFilePath();
    return _watcher->directories().contains(canonicalPath);
}

QStringList QGCFileWatcher::watchedDirectories() const
{
    return _watcher->directories();
}

// ============================================================================
// Bulk Operations
// ============================================================================

int QGCFileWatcher::watchFiles(const QStringList &filePaths, ChangeCallback callback)
{
    int count = 0;
    for (const QString &path : filePaths) {
        if (watchFile(path, callback)) {
            count++;
        }
    }
    return count;
}

int QGCFileWatcher::watchDirectories(const QStringList &directoryPaths, ChangeCallback callback)
{
    int count = 0;
    for (const QString &path : directoryPaths) {
        if (watchDirectory(path, callback)) {
            count++;
        }
    }
    return count;
}

void QGCFileWatcher::clear()
{
    const QStringList files = _watcher->files();
    if (!files.isEmpty()) {
        _watcher->removePaths(files);
    }

    const QStringList dirs = _watcher->directories();
    if (!dirs.isEmpty()) {
        _watcher->removePaths(dirs);
    }

    _fileCallbacks.clear();
    _directoryCallbacks.clear();
    _persistentFiles.clear();
    _pendingFileChanges.clear();
    _pendingDirectoryChanges.clear();
    _debounceTimer->stop();

    qCDebug(QGCFileWatcherLog) << "Cleared all watches";
}

// ============================================================================
// Persistent File Watching
// ============================================================================

bool QGCFileWatcher::watchFilePersistent(const QString &filePath, ChangeCallback callback)
{
    const QString canonicalPath = QFileInfo(filePath).absoluteFilePath();

    // Watch the parent directory to detect file recreation
    const QString parentDir = QFileInfo(canonicalPath).absolutePath();
    if (!_watcher->directories().contains(parentDir)) {
        _watcher->addPath(parentDir);
    }

    _persistentFiles.insert(canonicalPath);

    if (QFileInfo::exists(canonicalPath)) {
        return watchFile(canonicalPath, callback);
    }

    // File doesn't exist yet - store callback for when it's created
    if (callback) {
        _fileCallbacks[canonicalPath] = callback;
    }

    qCDebug(QGCFileWatcherLog) << "Watching file (persistent):" << canonicalPath;
    return true;
}

// ============================================================================
// Internal Slots
// ============================================================================

void QGCFileWatcher::_onFileChanged(const QString &path)
{
    qCDebug(QGCFileWatcherLog) << "File changed:" << path;

    // Handle persistent files that were deleted
    if (_persistentFiles.contains(path) && !QFileInfo::exists(path)) {
        // File was deleted - will be re-added when directory changes detect recreation
        qCDebug(QGCFileWatcherLog) << "Persistent file deleted, waiting for recreation:" << path;
    }

    _scheduleCallback(path, false);
}

void QGCFileWatcher::_onDirectoryChanged(const QString &path)
{
    qCDebug(QGCFileWatcherLog) << "Directory changed:" << path;

    // Check for recreated persistent files
    for (const QString &persistentPath : std::as_const(_persistentFiles)) {
        if (persistentPath.startsWith(path) && QFileInfo::exists(persistentPath)) {
            if (!_watcher->files().contains(persistentPath)) {
                _watcher->addPath(persistentPath);
                qCDebug(QGCFileWatcherLog) << "Re-added persistent file:" << persistentPath;

                // Trigger callback for recreated file
                _scheduleCallback(persistentPath, false);
            }
        }
    }

    _scheduleCallback(path, true);
}

void QGCFileWatcher::_scheduleCallback(const QString &path, bool isDirectory)
{
    if (isDirectory) {
        _pendingDirectoryChanges.insert(path);
    } else {
        _pendingFileChanges.insert(path);
    }

    if (_debounceDelay > 0) {
        if (!_debounceTimer->isActive()) {
            _debounceTimer->start(_debounceDelay);
        }
    } else {
        // No debounce - process immediately
        _processPendingChanges();
    }
}

void QGCFileWatcher::_processPendingChanges()
{
    // Process file changes
    for (const QString &path : std::as_const(_pendingFileChanges)) {
        emit fileChanged(path);

        auto it = _fileCallbacks.find(path);
        if (it != _fileCallbacks.end() && it.value()) {
            it.value()(path);
        }
    }
    _pendingFileChanges.clear();

    // Process directory changes
    for (const QString &path : std::as_const(_pendingDirectoryChanges)) {
        emit directoryChanged(path);

        auto it = _directoryCallbacks.find(path);
        if (it != _directoryCallbacks.end() && it.value()) {
            it.value()(path);
        }
    }
    _pendingDirectoryChanges.clear();
}
