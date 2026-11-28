#pragma once

#include <chrono>
#include <memory>

#include <QtCore/QString>

class QLockFile;

/// \brief A directory that one operation works in, beside user data, which a crash, a kill or a failed removal can
/// leave behind
///
/// The directory holds a lock file for as long as the object exists, so that removeStale() tells a directory that was
/// left behind from one in use, also by another QGC process. The object removes the directory when it is destroyed.
class QGCWorkDirectory
{
    Q_DISABLE_COPY_MOVE(QGCWorkDirectory)

public:
    /// Creating and locking a directory, and looking for stale ones, both hold a lock of the parent, so that a new
    /// directory is never seen before it is locked. Either waits this long for the other
    static constexpr std::chrono::milliseconds kParentLockTimeout{5000};

    /// Creates a new directory "<prefix>XXXXXX" in @a parentPath, which must exist, and locks it. Without the lock of
    /// the parent after @a parentLockTimeout it is created all the same
    QGCWorkDirectory(const QString& parentPath, const QString& prefix,
                     std::chrono::milliseconds parentLockTimeout = kParentLockTimeout);
    ~QGCWorkDirectory();

    /// Whether the directory was created and locked
    bool isValid() const { return !_path.isEmpty(); }

    QString path() const { return _path; }

    /// Why the directory could not be created, empty if it was
    QString errorString() const { return _errorString; }

    /// Removes every directory "<prefix>*" in @a parentPath whose lock is not held: one without a lock file, or one
    /// whose lock belongs to a process that has ended. A directory in use, also by another process, stays.
    /// Without the lock of the parent after @a parentLockTimeout nothing is looked for: a directory that is being
    /// created is not locked yet.
    /// @return How many directories were removed
    static int removeStale(const QString& parentPath, const QString& prefix,
                           std::chrono::milliseconds parentLockTimeout = kParentLockTimeout);

private:
    QString _path;
    QString _errorString;
    std::unique_ptr<QLockFile> _lock;
};
