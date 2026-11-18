#include "RunGuard.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RunGuardLog, "Utilities.RunGuard")

RunGuard::RunGuard(const QString &key)
    : _key(key)
    , _lockFilePath(lockDir() + QLatin1String("/qgc-") + generateKeyHash(key, QLatin1String("_lock")) + QLatin1String(".lock"))
    , _lockFile(_lockFilePath)
{
    // Recover instantly from stale locks after crashes.
    _lockFile.setStaleLockTime(0);
}

RunGuard::~RunGuard()
{
    release();
}

bool RunGuard::isAnotherRunning()
{
    if (_lockFile.isLocked()) {
        return false;
    }

    if (_lockFile.tryLock(0)) {
        _lockFile.unlock();
        return false;
    }

    switch (_lockFile.error()) {
    case QLockFile::NoError:
        return false;
    case QLockFile::LockFailedError:
        return true;
    case QLockFile::PermissionError:
        qCWarning(RunGuardLog) << "QLockFile PermissionError: Unable to access lock file at" << _lockFile.fileName();
        break;
    case QLockFile::UnknownError:
        qCWarning(RunGuardLog) << "QLockFile UnknownError: An unknown error occurred with lock file at"
                               << _lockFile.fileName();
        break;
    default:
        break;
    }

    return false;
}

bool RunGuard::tryToRun()
{
    if (_lockFile.isLocked() || _lockFile.tryLock(0)) {
        return true;
    }

    if (_lockFile.error() == QLockFile::LockFailedError) {
        return false;
    }

    // The lock file could not be created, which says nothing about other instances: do not block start-up
    qCWarning(RunGuardLog) << "Unable to create lock file" << _lockFile.fileName()
                           << "QLockFile error:" << _lockFile.error();
    return true;
}

void RunGuard::release()
{
    if (_lockFile.isLocked()) {
        _lockFile.unlock();
    }
}

QString RunGuard::generateKeyHash(const QString &key, const QString &salt)
{
    QByteArray data;
    (void) data.append(key.toUtf8());
    (void) data.append(salt.toUtf8());
    const QByteArray hex = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();
    return QString::fromLatin1(hex);
}

QString RunGuard::lockDir()
{
    QString dir;
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    // The temporary directory is shared between users here, and a user cannot remove another user's lock file.
    // On the other platforms it is per-user already.
    dir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
#endif
    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    }
    if (dir.isEmpty()) {
        dir = QDir::tempPath();
    }

    if (!QDir().mkpath(dir)) {
        qCWarning(RunGuardLog) << "Failed to create lock directory:" << dir;
        return QString();
    }

    return dir;
}
