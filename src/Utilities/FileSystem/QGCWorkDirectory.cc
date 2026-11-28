#include "QGCWorkDirectory.h"

#include <chrono>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLockFile>
#include <QtCore/QTemporaryDir>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCWorkDirectoryLog, "Utilities.QGCWorkDirectory")

namespace {

QString parentLockPath(const QString& parentPath, const QString& prefix)
{
    return QDir(parentPath).filePath(prefix + QStringLiteral("lock"));
}

/// Locks the work directory @a path, also when its lock is stale. Null if its lock is held
std::unique_ptr<QLockFile> lockWorkDirectory(const QString& path)
{
    auto lock = std::make_unique<QLockFile>(QDir(path).filePath(QStringLiteral(".lock")));
    // Only a lock whose process has ended is stale, however old it is
    lock->setStaleLockTime(std::chrono::milliseconds::zero());
    if (!lock->tryLock(std::chrono::milliseconds::zero())) {
        return nullptr;
    }
    return lock;
}

}  // namespace

QGCWorkDirectory::QGCWorkDirectory(const QString& parentPath, const QString& prefix,
                                   std::chrono::milliseconds parentLockTimeout)
{
    // Before the lock, which Qt warns about on Windows where it has no directory to be in
    if (!QFileInfo(parentPath).isDir()) {
        _errorString = QStringLiteral("The directory %1 does not exist").arg(parentPath);
        return;
    }

    QLockFile parentLock(parentLockPath(parentPath, prefix));
    parentLock.setStaleLockTime(std::chrono::milliseconds::zero());
    if (!parentLock.tryLock(parentLockTimeout)) {
        // Better to go on: removeStale() of another process could only remove the directory in the instant before it
        // is locked
        qCDebug(QGCWorkDirectoryLog) << "Creating a work directory without the lock of its parent:" << parentPath;
    }

    QTemporaryDir directory(QDir(parentPath).filePath(prefix + QStringLiteral("XXXXXX")));
    if (!directory.isValid()) {
        _errorString = directory.errorString();
        return;
    }
    _lock = lockWorkDirectory(directory.path());
    if (!_lock) {
        _errorString = QStringLiteral("Cannot lock the work directory");
        return;
    }
    directory.setAutoRemove(false);
    _path = directory.path();
}

QGCWorkDirectory::~QGCWorkDirectory()
{
    if (_path.isEmpty()) {
        return;
    }

    // The lock file goes first: Windows cannot remove a directory that holds an open file
    _lock.reset();
    if (!QDir(_path).removeRecursively()) {
        qCWarning(QGCWorkDirectoryLog) << "Could not remove a work directory, it is removed as a stale one later:"
                                       << _path;
    }
}

int QGCWorkDirectory::removeStale(const QString& parentPath, const QString& prefix,
                                  std::chrono::milliseconds parentLockTimeout)
{
    if (!QFileInfo(parentPath).isDir()) {
        return 0;
    }

    QLockFile parentLock(parentLockPath(parentPath, prefix));
    parentLock.setStaleLockTime(std::chrono::milliseconds::zero());
    if (!parentLock.tryLock(parentLockTimeout)) {
        qCDebug(QGCWorkDirectoryLog) << "Stale work directories not looked for, the parent is locked:" << parentPath;
        return 0;
    }

    int removed = 0;
    const QDir parent(parentPath);
    // A link is not followed: removing what it leads to would remove what is not a work directory
    const QStringList names = parent.entryList({prefix + QLatin1Char('*')},
                                               QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    for (const QString& name : names) {
        const QString path = parent.filePath(name);
        std::unique_ptr<QLockFile> lock = lockWorkDirectory(path);
        if (!lock) {
            continue;
        }
        lock.reset();
        if (QDir(path).removeRecursively()) {
            qCDebug(QGCWorkDirectoryLog) << "Removed a stale work directory:" << path;
            ++removed;
        } else {
            qCWarning(QGCWorkDirectoryLog) << "Could not remove a stale work directory:" << path;
        }
    }
    return removed;
}
